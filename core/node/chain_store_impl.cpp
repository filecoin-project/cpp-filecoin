/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "chain_store_impl.hpp"

#include "chain_db.hpp"
#include "common/logger.hpp"
#include "events.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {

  using primitives::cid::getCidOfCbor;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("chain_store");
      return logger.get();
    }
  }  // namespace

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<ChainDb> chain_db,
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
      std::shared_ptr<CachedInterpreter> interpreter,
      std::shared_ptr<WeightCalculator> weight_calculator,
      std::shared_ptr<BlockValidator> block_validator)
      : chain_db_(std::move(chain_db)),
        ipld_(std::move(ipld)),
        interpreter_(std::move(interpreter)),
        weight_calculator_(std::move(weight_calculator)),
        block_validator_(std::move(block_validator)) {
    assert(chain_db_);
    assert(ipld_);
    assert(interpreter_);
    assert(weight_calculator_);
    assert(block_validator_);
  }

  outcome::result<void> ChainStoreImpl::start(
      std::shared_ptr<events::Events> events, std::string network_name) {
    events_ = std::move(events);
    network_name_ = std::move(network_name);
    assert(events_);

    head_constructor_.start(events_);

    head_interpreted_event_ = events_->subscribeHeadInterpreted(
        [this](const events::HeadInterpreted &e) { onHeadInterpreted(e); });

    OUTCOME_TRY(chain_db_->start(
        [wptr = weak_from_this()](std::vector<TipsetHash> removed,
                                  std::vector<TipsetHash> added) {
          auto self = wptr.lock();
          if (self) {
            self->headsChangedInStore(std::move(removed), std::move(added));
          }
        }));

    OUTCOME_TRY(chain_db_->getHeads(
        [&](std::vector<TipsetHash> removed, std::vector<TipsetHash> added) {
          headsChangedInStore(std::move(removed), std::move(added));
        }));

    if (!head_ && possible_heads_.empty()) {
      // at least genesis must be chosen
      return storage::blockchain::ChainStoreError::kNoHeaviestTipset;
    }

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(cid, ipld_->setCbor(block));
    head_constructor_.blockFromApi(cid, block);
    return outcome::success();
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::loadTipset(
      const TipsetHash &hash) {
    return chain_db_->getTipsetByHash(hash);
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::loadTipset(const TipsetKey &key) {
    return chain_db_->getTipsetByKey(key);
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::loadTipsetByHeight(
      uint64_t height) {
    return chain_db_->getTipsetByHeight(height);
  }

  TipsetCPtr ChainStoreImpl::heaviestTipset() const {
    assert(head_);
    return head_;
  }

  storage::blockchain::ChainStore::connection_t
  ChainStoreImpl::subscribeHeadChanges(
      const std::function<HeadChangeSignature> &subscriber) {
    if (head_) {
      subscriber(events::HeadChange{primitives::tipset::HeadChangeType::CURRENT,
                                    head_});
    }
    return head_change_signal_.connect(subscriber);
  }

  const std::string &ChainStoreImpl::getNetworkName() const {
    return network_name_;
  }

  const CID &ChainStoreImpl::genesisCID() const {
    return chain_db_->genesisCID();
  }

  primitives::BigInt ChainStoreImpl::getHeaviestWeight() const {
    return heaviest_weight_;
  }

  ChainStoreImpl::HeadInfo ChainStoreImpl::getHeadInfo(const TipsetHash &h) {
    HeadInfo info;
    auto res = chain_db_->getTipsetByHash(h);
    if (res) {
      // tipset is stored
      info.tipset = std::move(res.value());

      auto interpret_res = interpreter_->getCached(info.tipset->key);
      if (!interpret_res) {
        // bad tipset or storage error
        info.is_bad = true;
      } else if (interpret_res.value().has_value()) {
        // already interpreted, calculate weight
        auto weight_res = weight_calculator_->calculateWeight(*info.tipset);
        if (!weight_res) {
          log()->error("tipset weight calc error: {}",
                       weight_res.error().message());
          info.is_bad = true;
        } else {
          info.weight = std::move(weight_res.value());
        }
      }
    } else {
      // the tipset must be in storage anyways if its hash got into this fn
      log()->error("chain store inconsistent, cannot extract stored tipset: {}",
                   res.error().message());
      info.is_bad = true;
    }

    if (!info.is_bad && info.weight == 0) {
      // this head has been downloaded, but not interpreted/validated,
      // lets do it in async manner (and wait for HeadInterpreted)
      events_->signalHeadDownloaded({info.tipset});
    }

    return info;
  }

  void ChainStoreImpl::headsChangedInStore(std::vector<TipsetHash> removed,
                                           std::vector<TipsetHash> added) {
    for (const auto &h : removed) {
      possible_heads_.erase(h);
    }

    for (const auto &h : added) {
      possible_heads_.insert({h, getHeadInfo(h)});
    }

    // attempt to choose new head
    BigInt weight = heaviest_weight_;
    TipsetCPtr new_head_chosen;
    for (const auto &[_, info] : possible_heads_) {
      if (!info.is_bad && info.weight > weight) {
        weight = info.weight;
        new_head_chosen = info.tipset;
      }
    }

    if (new_head_chosen) {
      newHeadChosen(std::move(new_head_chosen), std::move(weight));
    }
  }

  void ChainStoreImpl::onHeadInterpreted(const events::HeadInterpreted &e) {
    auto &info = possible_heads_[e.head->key.hash()];

    if (!info.tipset) {
      // also possible
      info.tipset = e.head;
    }

    if (e.result) {
      auto weight_res = weight_calculator_->calculateWeight(*e.head);
      if (!weight_res) {
        log()->error("tipset weight calc error: {}",
                     weight_res.error().message());
        info.is_bad = true;
      } else {
        info.weight = std::move(weight_res.value());
        if (info.weight > heaviest_weight_) {
          newHeadChosen(e.head, info.weight);
        }
      }
    }
  }

  void ChainStoreImpl::newHeadChosen(TipsetCPtr tipset, BigInt weight) {
    using primitives::tipset::HeadChange;
    using primitives::tipset::HeadChangeType;

    if (weight <= heaviest_weight_) {
      // should not get here
      return;
    }

    try {
      HeadChange event;

      bool first_head_apply = (head_ == nullptr);

      if (!first_head_apply) {
        assert(*head_ != *tipset);

        auto res = chain_db_->findHighestCommonAncestor(head_, tipset);
        if (!res) {
          log()->error(
              "findHighestCommonAncestor failed: {}, ignoring new head!",
              res.error().message());
          return;
        }

        auto &ancestor = res.value();
        assert(ancestor);
        auto ancestor_height = ancestor->height();

        if (*ancestor != *head_) {
          assert(ancestor_height < head_->height());

          log()->info("fork detected, reverting from height {} to {}!",
                      head_->height(),
                      ancestor_height);

          event.type = HeadChangeType::REVERT;

          OUTCOME_EXCEPT(chain_db_->walkBackward(
              head_->key.hash(), ancestor_height, [&, this](TipsetCPtr t) {
                if (t->height() <= ancestor_height) {
                  return false;
                }
                log()->debug("reverting h={}", t->height());
                event.value = t;
                head_change_signal_(event);
                events_->signalHeadChange(event);
                return true;
              }));
        }

        event.type = HeadChangeType::APPLY;

        assert(ancestor_height < tipset->height());

        OUTCOME_EXCEPT(chain_db_->walkForward(
            ancestor,
            tipset,
            tipset->height() - ancestor_height,
            [&, this](TipsetCPtr t) {
              auto h = t->height();
              if (h > tipset->height()) {
                return false;
              }
              if (h > ancestor_height) {
                log()->debug("applying h={}", t->height());
                event.value = t;
                head_change_signal_(event);
                events_->signalHeadChange(event);
              }
              return true;
            }));
      }

      OUTCOME_EXCEPT(chain_db_->setCurrentHead(tipset->key.hash()));

      log()->info(
          "new current head: height={}, weight={}", tipset->height(), weight);

      event.type =
          first_head_apply ? HeadChangeType::CURRENT : HeadChangeType::APPLY;
      event.value = tipset;
      head_change_signal_(event);
      events_->signalHeadChange(event);
      head_ = std::move(tipset);
      heaviest_weight_ = std::move(weight);
    } catch (const std::system_error &e) {
      log()->error("cannot apply new head, {}", e.code().message());
    }

    events_->signalCurrentHead({.tipset = head_, .weight = heaviest_weight_});
  }

}  // namespace fc::sync

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using E = fc::storage::blockchain::ChainStoreError;
  switch (e) {
    case E::kStoreNotInitialized:
      return "chain store error: not initialized";
    case E::kNoHeaviestTipset:
      return "chain store error: no heaviest tipset";
    case E::kNoTipsetAtHeight:
      return "chain store error: no tipset at required height";
    case E::kBlockRejected:
      return "chain store error: block rejected";
    case E::kIllegalState:
      return "chain store error: illegal state";
    default:
      break;
  }
  return "ChainStoreError: unknown error";
}
