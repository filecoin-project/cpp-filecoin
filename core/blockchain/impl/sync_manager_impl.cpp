/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_manager_impl.hpp"

#include <boost/assert.hpp>
#include <gsl/gsl_util>
#include "primitives/cid/json_codec.hpp"

namespace fc::blockchain::sync_manager {

  SyncManagerImpl::SyncManagerImpl(boost::asio::io_context &context,
                                   SyncFunction sync_function)
      : peer_heads_{},
        state_{BootstrapState::STATE_INIT},
        bootstrap_threshold_{1},
        sync_targets_{},
        sync_results_{},
        incoming_tipsets_{},
        active_syncs_{},
        sync_function_(std::move(sync_function)),
        logger_{common::createLogger("SyncManager")} {
    BOOST_ASSERT_MSG(static_cast<bool>(sync_function_),
                     "sync function is not callable");
  }

  BootstrapState SyncManagerImpl::getBootstrapState() const {
    return state_;
  }

  void SyncManagerImpl::setBootstrapState(BootstrapState state) {
    state_ = state;
  }

  bool SyncManagerImpl::isBootstrapped() const {
    return state_ == BootstrapState::STATE_COMPLETE;
  }

  size_t SyncManagerImpl::syncedPeerCount() const {
    size_t count = 0;
    for (auto &[key, val] : peer_heads_) {
      if (val.height > 0) ++count;
    }

    return count;
  }

  outcome::result<SyncManagerImpl::Tipset> SyncManagerImpl::selectSyncTarget() {
    SyncBucketSet buckets(std::vector<Tipset>{});
    std::vector<Tipset> peer_heads;
    for (auto &[_, ts] : peer_heads_) {
      peer_heads.push_back(ts);
    }

    std::sort(peer_heads.begin(),
              peer_heads.end(),
              [](const Tipset &l, const Tipset &r) -> bool {
                return l.height < r.height;
              });
    for (auto &ts : peer_heads) {
      buckets.insert(ts);
    }

    if (buckets.getSize() > 1) {
      logger_->warn(
          "caution, multiple distinct chains seen during head selections");
    }

    return buckets.getHeaviestTipset();
  }

  outcome::result<void> SyncManagerImpl::processResult(
      const SyncResult &result) {
    if (result.success
        && getBootstrapState() != BootstrapState::STATE_COMPLETE) {
      setBootstrapState(BootstrapState::STATE_COMPLETE);
    }
    OUTCOME_TRY(key, result.tipset.makeKey());
    active_syncs_.erase(key);

    auto &&related_bucket = active_sync_tips_.popRelated(result.tipset);
    if (related_bucket != boost::none) {
      if (result.success) {
        if (next_sync_target_ == boost::none) {
          next_sync_target_ = related_bucket;
          OUTCOME_TRY(
              processSyncTargets(next_sync_target_->getHeaviestTipset()));
        } else {
          sync_queue_.append(*related_bucket);
        }
        return outcome::success();
      } else {
        // TODO: this is the case where we try to sync a chain, and
        // fail, and we have more blocks on top of that chain that
        // have come in since.  The question is, should we try to
        // sync these? or just drop them?
      }
    }
    if (next_sync_target_ == boost::none && !sync_queue_.isEmpty()) {
      auto &&target = sync_queue_.pop();
      if (target != boost::none) {
        next_sync_target_ = std::move(target);
        OUTCOME_TRY(processSyncTargets(next_sync_target_->getHeaviestTipset()));
      }
    }

    return outcome::success();
  }

  outcome::result<void> SyncManagerImpl::processIncomingTipset(
      const Tipset &tipset) {
    OUTCOME_TRY(cids_json, codec::json::encodeCidVector(tipset.cids));
    logger_->info("scheduling incoming tipset sync %s", cids_json);

    if (getBootstrapState() == BootstrapState::STATE_SELECTED) {
      setBootstrapState(BootstrapState::STATE_SCHEDULED);
      sync_targets_.push_back(tipset);
      // process sync_targets update
    }

    bool is_related_to_active_sync = false;
    for (auto &[_, acts] : active_syncs_) {
      if (tipset == acts) break;
      OUTCOME_TRY(parents, tipset.getParents());
      OUTCOME_TRY(key, acts.makeKey());
      if (parents == key) {
        is_related_to_active_sync = true;
        break;
      }
    }

    is_related_to_active_sync |= active_sync_tips_.isRelatedToAny(tipset);
    if (is_related_to_active_sync) {
      active_sync_tips_.insert(tipset);
      return outcome::success();
    }

    if (getBootstrapState() == BootstrapState::STATE_SCHEDULED) {
      sync_queue_.insert(tipset);
      return outcome::success();
    }

    if (next_sync_target_ != boost::none
        && next_sync_target_->isSameChain(tipset)) {
      next_sync_target_->addTipset(tipset);
    } else {
      sync_queue_.insert(tipset);
      if (next_sync_target_ == boost::none) {
        next_sync_target_ = sync_queue_.pop();
        OUTCOME_TRY(processSyncTargets(next_sync_target_->getHeaviestTipset()));
      }
    }

    return outcome::success();
  }

  outcome::result<void> SyncManagerImpl::doSync() {
    while(!sync_targets_.empty()) {
      auto &&ts = std::move(sync_targets_.front());
      sync_targets_.pop_front();
      // do external sync
      auto &&res = sync_function_(ts);
      if (!res) {
        logger_->error("sync error %s", res.error().message());
      }

      SyncResult sync_result{std::move(ts), res};
      if (auto &&result = processResult(sync_result); !result) {
        logger_->error("sync failed: " + result.error().message());
      }
    }

    return outcome::success();
  }

  // worker
  outcome::result<void> SyncManagerImpl::processSyncTargets(Tipset ts) {
    // schedule work sent
    OUTCOME_TRY(key, ts.makeKey());
    active_syncs_[key] = ts;
    if (!sync_queue_.isEmpty()) {
      next_sync_target_ = sync_queue_.pop();
    } else {
      next_sync_target_ = boost::none;
    }
    // do sync
    sync_targets_.push_back(std::move(ts));
    return doSync();
  }

  outcome::result<void> SyncManagerImpl::setPeerHead(PeerId peer_id,
                                                     const Tipset &tipset) {
    peer_heads_[peer_id] = tipset;
    auto state = state_;
    switch (state) {
      case BootstrapState::STATE_INIT: {
        auto synced_count = syncedPeerCount();
        if (synced_count >= bootstrap_threshold_) {
          auto &&target = selectSyncTarget();
          if (!target) {
            logger_->error("failed to select sync target: %s",
                           target.error().message());
            return target.error();
          }
          state_ = BootstrapState::STATE_SELECTED;
          return processIncomingTipset(target.value());
        }
        logger_->info("sync bootstrap has %d peers", synced_count);
        return outcome::success();
      }
      case BootstrapState::STATE_SELECTED:
      case BootstrapState::STATE_SCHEDULED:
      case BootstrapState::STATE_COMPLETE:
        return processIncomingTipset(tipset);
    }
  }

}  // namespace fc::blockchain::sync_manager

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::sync_manager, SyncManagerError, e) {
  // SHUTTING_DOWN
  switch (e) {
    using Error = fc::blockchain::sync_manager::SyncManagerError;
    case Error::SHUTTING_DOWN:
      return "shutting down";
    case Error::NO_SYNC_TARGET:
      return "no sync target present";
  }
}
