/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_manager_impl.hpp"

#include <boost/assert.hpp>

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

  // 	var buckets syncBucketSet
  //
  //	var peerHeads []*types.TipSet
  //	for _, ts := range sm.peerHeads {
  //		peerHeads = append(peerHeads, ts)
  //	}
  //	sort.Slice(peerHeads, func(i, j int) bool {
  //		return peerHeads[i].Height() < peerHeads[j].Height()
  //	})
  //
  //	for _, ts := range peerHeads {
  //		buckets.Insert(ts)
  //	}
  //
  //	if len(buckets.buckets) > 1 {
  //		log.Warn("caution, multiple distinct chains seen during head
  //selections")
  //		// TODO: we *could* refuse to sync here without user
  //intervention.
  //		// For now, just select the best cluster
  //	}
  //
  //	return buckets.Heaviest(), nil

  // outcome::result<SyncManagerImpl::Tipset>
  // SyncManagerImpl::selectSyncTarget() {
  //
  //    SyncBucketSet buckets(std::vector<Tipset>{});
  //    std::vector<Tipset> peer_heads;
  //    for (auto &[_, ts] : peer_heads_) {
  //      peer_heads.push_back(ts);
  //    }
  //
  //    return {};
  //  }

  void SyncManagerImpl::processResult(const SyncResult &result) {}
  // log.Info("scheduling incoming tipset sync: ", ts.Cids())
  //	if sm.getBootstrapState() == BSStateSelected {
  //		sm.setBootstrapState(BSStateScheduled)
  //		sm.syncTargets <- ts
  //		return
  //	}
  //
  //	var relatedToActiveSync bool
  //	for _, acts := range sm.activeSyncs {
  //		if ts.Equals(acts) {
  //			break
  //		}
  //
  //		if ts.Parents() == acts.Key() {
  //			// sync this next, after that sync process finishes
  //			relatedToActiveSync = true
  //		}
  //	}
  //
  //	if !relatedToActiveSync && sm.activeSyncTips.RelatedToAny(ts) {
  //		relatedToActiveSync = true
  //	}
  //
  //	// if this is related to an active sync process, immediately bucket it
  //	// we don't want to start a parallel sync process that duplicates work
  //	if relatedToActiveSync {
  //		sm.activeSyncTips.Insert(ts)
  //		return
  //	}
  //
  //	if sm.getBootstrapState() == BSStateScheduled {
  //		sm.syncQueue.Insert(ts)
  //		return
  //	}
  //
  //	if sm.nextSyncTarget != nil && sm.nextSyncTarget.sameChainAs(ts) {
  //		sm.nextSyncTarget.add(ts)
  //	} else {
  //		sm.syncQueue.Insert(ts)
  //
  //		if sm.nextSyncTarget == nil {
  //			sm.nextSyncTarget = sm.syncQueue.Pop()
  //			sm.workerChan = sm.syncTargets
  //		}
  //	}

  outcome::result<void> SyncManagerImpl::processIncomingTipset(
      const Tipset &tipset) {
    return outcome::success();
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
  }
}
