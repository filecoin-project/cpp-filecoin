/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_manager_impl.hpp"

#include <gsl/gsl_util>
#include "blockchain/chain_synchronizer.hpp"
#include "primitives/cid/json_codec.hpp"

namespace fc::blockchain {

  SyncManagerImpl::SyncManagerImpl(io_context &context,
                                   PeerId self_id,
                                   std::shared_ptr<Sync> sync,
                                   std::shared_ptr<ChainStore> chain_store,
                                   std::shared_ptr<IpfsDatastore> block_store)
      : context_(context),
        self_peer_id_(std::move(self_id)),
        sync_(std::move(sync)),
        chain_store_(std::move(chain_store)),
        block_store_(std::move(block_store)) {
    setState(BootstrapState::STATE_INIT);
  }

  outcome::result<bool> SyncManagerImpl::onHeadReceived(
      const HeadMessage &msg) {
    primitives::tipset::TipsetKey tipset_key{msg.blocks};

    switch (getState()) {
      case BootstrapState::STATE_INIT: {
        // if chainstore has tipset set online  and continue
        OUTCOME_TRY(contains, chain_store_->containsTipset(tipset_key));
        if (contains) {
          setState(BootstrapState::STATE_COMPLETE);
          sync_state_changed_({true, tipset_key, msg.weight});
        } else {
          setState(BootstrapState::STATE_SYNCHRONIZING);
          sync_state_changed_({false, tipset_key, msg.weight});
          startSynchronizer(tipset_key);
        }

        return true;
      }
      case BootstrapState::STATE_SYNCHRONIZING: {
        OUTCOME_TRY(contains, chain_store_->containsTipset(tipset_key));
        if (contains) {
          return true;  // chain already contains tipset, nothing changes
        }

        sync_state_changed_({false, tipset_key, msg.weight});
        startSynchronizer(tipset_key);
      }
      case BootstrapState::STATE_COMPLETE: {
        // for now ignore, add re-synchronization in future
      }
    }

    return true;
  }

  void SyncManagerImpl::startSynchronizer(const TipsetKey &key) {
    auto &synchronizer =
        synchronizers_.emplace_back(std::make_shared<ChainSynchronizer>(
            context_, weak_from_this(), sync_, chain_store_));
    synchronizer->start(key, kChainItemsLimit);
  }
}  // namespace fc::blockchain

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain, SyncManagerError, e) {
  // SHUTTING_DOWN
  switch (e) {
    using Error = fc::blockchain::SyncManagerError;
    case Error::SHUTTING_DOWN:
      return "shutting down";
    case Error::NO_SYNC_TARGET:
      return "no sync target present";
  }

  return "unknown SyncManager error";
}
