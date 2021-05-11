/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/chain_store_impl.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "node/events.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("chain_store");
      return logger.get();
    }
  }  // namespace

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
      TsLoadPtr ts_load,
      TipsetCPtr head,
      BigInt weight,
      std::shared_ptr<BlockValidator> block_validator)
      : ipld_(std::move(ipld)),
        ts_load_(std::move(ts_load)),
        block_validator_(std::move(block_validator)) {
    assert(ipld_);
    assert(block_validator_);
    head_ = head;
    heaviest_weight_ = weight;
  }

  outcome::result<void> ChainStoreImpl::start(
      std::shared_ptr<events::Events> events) {
    events_ = std::move(events);
    assert(events_);

    head_constructor_.start(events_);

    events_->signalCurrentHead({.tipset = head_, .weight = heaviest_weight_});

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(cid, ipld_->setCbor(block));
    head_constructor_.blockFromApi(cid, block);
    return outcome::success();
  }

  TipsetCPtr ChainStoreImpl::heaviestTipset() const {
    assert(head_);
    return head_;
  }

  storage::blockchain::ChainStore::connection_t
  ChainStoreImpl::subscribeHeadChanges(
      const std::function<HeadChangeSignature> &subscriber) {
    assert(head_);
    subscriber(primitives::tipset::HeadChange{
        primitives::tipset::HeadChangeType::CURRENT, head_});
    return head_change_signal_.connect(subscriber);
  }

  primitives::BigInt ChainStoreImpl::getHeaviestWeight() const {
    return heaviest_weight_;
  }

  void ChainStoreImpl::update(Path &path, const BigInt &weight) {
    auto &[revert, apply]{path};
    using primitives::tipset::HeadChange;
    using primitives::tipset::HeadChangeType;
    HeadChange event;
    auto notify{[&](auto &it) {
      if (auto _ts{ts_load_->lazyLoad(it->second)}) {
        event.value = _ts.value();
        head_change_signal_(event);
      } else {
        log()->error("update ts_load {:#}", _ts.error());
      }
    }};
    event.type = HeadChangeType::REVERT;
    for (auto it{std::prev(revert.end())}; it != revert.begin(); --it) {
      notify(it);
    }
    event.type = HeadChangeType::APPLY;
    for (auto it{std::next(apply.begin())}; it != apply.end(); ++it) {
      notify(it);
    }
    head_ = ts_load_->lazyLoad(std::prev(apply.end())->second).value();
    heaviest_weight_ = weight;
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
