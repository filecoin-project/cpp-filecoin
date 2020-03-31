/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP

#include "blockchain/sync_manager.hpp"

#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include "blockchain/impl/sync_bucket_set.hpp"
#include "blockchain/syncer_state.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "primitives/tipset/tipset.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::blockchain {
  /** @brief SyncManager error */
  enum class SyncManagerError { SHUTTING_DOWN = 1, NO_SYNC_TARGET };

  class SyncManagerImpl : public SyncManager,
                          std::enable_shared_from_this<SyncManager> {
   public:
    SyncManagerImpl(io_context &context,
                    PeerId self_id,
                    std::shared_ptr<Sync> sync,
                    std::shared_ptr<ChainStore> chain_store,
                    std::shared_ptr<IpfsDatastore> block_store);

    ~SyncManagerImpl() override = default;

    outcome::result<bool> onHeadReceived(const HeadMessage &msg) override;

    virtual bool isPlannedDownload(const CID &object) const = 0;

    virtual std::shared_ptr<ChainSynchronizer> findRelatedChain(
        const TipsetKey &key) = 0;

    void startSynchronizer(const TipsetKey &key) override;

    using SyncStateChangedFn = void(const SyncState &);

    boost::signals2::connection connectSyncStateChanged(SyncStateChangedFn cb) {
      return sync_state_changed_.connect(cb);
    }

   private:
    // TODO (yuraz): get this value from config
    const size_t kChainItemsLimit = 10u;
    io_context &context_;
    PeerId self_peer_id_;
    std::shared_ptr<Sync> sync_;
    std::shared_ptr<ChainStore> chain_store_;
    std::shared_ptr<IpfsDatastore> block_store_;
    std::shared_ptr<IpfsDatastore> bad_blocks_;
    std::list<std::shared_ptr<ChainSynchronizer>> synchronizers_;
    boost::signals2::signal<SyncStateChangedFn> sync_state_changed_;
    common::Logger logger_;
  };
}  // namespace fc::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain, SyncManagerError);

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
