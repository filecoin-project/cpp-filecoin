/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_manager_impl.hpp"

#include <boost/assert.hpp>

namespace fc::blockchain {

  SyncManagerImpl::SyncManagerImpl(boost::asio::io_context &context,
                                   SyncFunction sync_function)
      : context_{context},
        peer_heads_{},
        state_{BootstrapState::STATE_INIT},
        bootstrap_threshold_{1},
        sync_targets_{},
        sync_results_{},
        sync_states_(kSyncWorkerCount),
        incoming_tipsets_{},
        active_syncs_{},
        sync_function_(std::move(sync_function)) {
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

  void SyncManagerImpl::workerMethod(int id) {
    // func (sm *SyncManager) syncWorker(id int) {
    //	ss := &SyncerState{}
    //	sm.syncStates[id] = ss
    //	for {
    //		select {
    //		case ts, ok := <-sm.syncTargets:
    //			if !ok {
    //				log.Info("sync manager worker shutting down")
    //				return
    //			}
    //
    //			ctx := context.WithValue(context.TODO(), syncStateKey{},
    // ss) 			err := sm.doSync(ctx, ts) 			if err != nil
    // { log.Errorf("sync error:
    //%+v", err)
    //			}
    //
    //			sm.syncResults <- &syncResult{
    //				ts:      ts,
    //				success: err == nil,
    //			}
    //		}
    //	}

    SyncerState ss{};
    sync_states_[id] = ss;
    // wait for tipset from sync_targets
    // put sync



    scheduleWorker(id);
  }

  void SyncManagerImpl::scheduleWorker(int id) {
    context_.post([self = this->shared_from_this(), id]() mutable {
      self->workerMethod(id);
    });
  }

}  // namespace fc::blockchain
