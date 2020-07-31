/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/impl/chain_store_impl.hpp"

#include "common/logger.hpp"

#include "sync/peer_manager.hpp"
#include "sync/sync_job.hpp"

namespace fc::storage::blockchain {

  using primitives::address::Address;
  using primitives::tipset::Tipset;

  using primitives::BigInt;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("chainstore");
      return logger.get();
    }

  }  // namespace

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    if (block.height != current_epoch_) {
      return ChainStoreError::kBlockRejected;
    }

    OUTCOME_TRY(parent_key, TipsetKey::create(block.parents));

    // TODO validate block even if its parent is not yet in chain_db,
    // it is possible that parent will be catched by syncer during this epoch

    // TODO ensure block messages are inside ipld store (???)
    // extract from blocksync_common.cpp storeBlock (???) to store msg meta

    auto &creator = head_candidates_[parent_key.hash()];

    // note, 2 calls due to performance, not to spoil the creator
    OUTCOME_TRY(creator.canExpandTipset(block));
    OUTCOME_TRY(cid, creator.expandTipset(block));

    // TODO store block into ipld, to make sure the tipset is loaded w/o errors
    // see TODO above

    if (head_
        && (head_->key.hash() == parent_key.hash()
            || heads_parent_ == parent_key.hash())) {
      TipsetCPtr new_hot_head = creator.getTipset(false);
      OUTCOME_TRY(need_to_sync_down_from,
                  chain_db_->storeTipset(new_hot_head, parent_key));
      if (need_to_sync_down_from) {
        // this is illegal state
        log()->error("illegal state, head must be synced!");
        return ChainStoreError::kIllegalState;
      }
    } else {
      log()->info("need to sync down from {}", parent_key.toPrettyString());
      // TODO new sync job from parent hash down. Dont store tipset at this
      // moment
    }

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
    if (head_) {
      auto current_height = head_->height();
      if (height == current_height) {
        // head_ may be not yet indexed, in progress
        return head_;
      } else if (height < current_height) {
        return chain_db_->getTipsetByHeight(height);
      }
    }
    return ChainStoreError::kNoTipsetAtHeight;
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::heaviestTipset() const {
    if (!head_) {
      return ChainStoreError::kNoHeaviestTipset;
    }
    return head_;
  }

  ChainStore::connection_t ChainStoreImpl::subscribeHeadChanges(
      const std::function<HeadChangeSignature> &subscriber) {
    if (head_) {
      // TODO take a look into ChainNotify api impl.
      // Is it acceptable to make callback before Chan{} is created?

      subscriber(HeadChange{.type = HeadChangeType::CURRENT, .value = head_});
    }
    return head_change_signal_.connect(subscriber);
  }

  const std::string &ChainStoreImpl::getNetworkName() const {
    return network_name_;
  }

  const CID &ChainStoreImpl::genesisCID() const {
    return chain_db_->genesisCID();
  }

  void ChainStoreImpl::onHeadsChanged(std::vector<TipsetHash> removed,
                                      std::vector<TipsetHash> added) {
    if (added.empty()) {
      // TODO this is NYI, cleanup of old heads takes place
      return;
    }

    try {
      auto [tipset, weight] = chooseNewHead(added);

      if (!tipset) {
        // heads are worse than present
        return;
      }

      // now we can switch heads

      // TODO there are variants! Check the epoch

      switchToHead(std::move(tipset), std::move(weight));

    } catch (const std::system_error &e) {
      log()->error("error in onHeadChanged callback", e.what());
    }
  }

  std::pair<TipsetCPtr, BigInt> ChainStoreImpl::chooseNewHead(
      const std::vector<TipsetHash> &heads_added) {
    auto max_weight = current_weight_;
    TipsetCPtr choice;
    for (const auto &hash : heads_added) {
      OUTCOME_EXCEPT(head, chain_db_->getTipsetByHash(hash));
      OUTCOME_EXCEPT(weight, weight_calculator_->calculateWeight(*head));
      if (weight > max_weight) {
        max_weight = weight;
        choice = head;
      }
    }
    return {choice, max_weight};
  }

  void ChainStoreImpl::switchToHead(TipsetCPtr new_head, BigInt new_weight) {
    OUTCOME_EXCEPT(parent_key, new_head->getParents());
    if (!head_) {
      // initial head apply
      head_ = new_head;
      heads_parent_ = parent_key.hash();
      current_weight_ = new_weight;
      head_change_signal_(
          HeadChange{.type = HeadChangeType::CURRENT, .value = head_});

    } else if (head_->key.hash() == parent_key.hash()) {
      // current head is father of new head
      head_ = new_head;
      heads_parent_ = parent_key.hash();
      current_weight_ = new_weight;
      head_change_signal_(
          HeadChange{.type = HeadChangeType::APPLY, .value = head_});
    } else if (head_->key.hash() == heads_parent_) {
      // current head is brother of new head
      // reentrancy warning: no current head during revert process otherwise we
      // need to call the notification on every step of revert or apply
      auto brother = std::move(head_);
      current_weight_ = 0;
      head_change_signal_(
          HeadChange{.type = HeadChangeType::REVERT, .value = brother});
      head_change_signal_(
          HeadChange{.type = HeadChangeType::APPLY, .value = head_});
      head_ = new_head;
      current_weight_ = new_weight;
    } else {
      // general case fork
      OUTCOME_EXCEPT(ancestor,
                     chain_db_->findHighestCommonAncestor(head_, new_head));
      if (!ancestor || ancestor == new_head) {
        outcome::raise(ChainStoreError::kIllegalState);
      }

      // TODO also think about reentrancy issues, maybe its acceptable not to
      // null our state
      auto old_head = std::move(head_);
      current_weight_ = 0;

      if (ancestor != head_) {
        // need to revert from head to ancestor
        head_change_signal_(
            HeadChange{.type = HeadChangeType::REVERT, .value = old_head});

        OUTCOME_EXCEPT(chain_db_->walkBackward(
            heads_parent_, ancestor->height(), [this](TipsetCPtr tipset) {
              head_change_signal_(HeadChange{.type = HeadChangeType::REVERT,
                                             .value = std::move(tipset)});
            }));
      }

      // now go forward
      OUTCOME_EXCEPT(chain_db_->walkForward(ancestor->height() + 1,
                                            new_head->height(),
                                            [this](TipsetCPtr tipset) {
                                              head_change_signal_(HeadChange{
                                                  .type = HeadChangeType::APPLY,
                                                  .value = std::move(tipset)});
                                            }));
      head_ = new_head;
      heads_parent_ = parent_key.hash();
      current_weight_ = new_weight;
    }

    syncer_->setCurrentWeightAndHeight(current_weight_, head_->height());
  }

  void ChainStoreImpl::onChainEpochTimer(Height new_epoch) {
    assert(new_epoch > current_epoch_);

    current_epoch_ = new_epoch;
    head_candidates_.clear();
  }

}  // namespace fc::storage::blockchain

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using fc::storage::blockchain::ChainStoreError;

  switch (e) {
    case ChainStoreError::kNoHeaviestTipset:
      return "ChainStore: no heaviest tipset yet";
    case ChainStoreError::kNoTipsetAtHeight:
      return "ChainStore: no tipset at given height";
    case ChainStoreError::kBlockRejected:
      return "ChainStore: block rejected";
    case ChainStoreError::kStoreNotInitialized:
      return "ChainStore: not initialized";
    case ChainStoreError::kIllegalState:
      return "ChainStore: illegal state";
  }

  return "ChainStoreError: unknown error";
}
