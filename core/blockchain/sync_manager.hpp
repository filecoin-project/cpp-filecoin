/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP

#include <functional>

#include <boost/asio/io_context.hpp>
#include <boost/signals2/signal.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include "blockchain/sync_manager.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "common/stateful.hpp"
#include "network/sync/sync.hpp"
#include "network/sync/sync_observer.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "storage/chain/block_store.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/ipfs/impl/ipfs_block_service.hpp"

namespace fc::blockchain {

  /**
   * @struct HeadMessage contains information about peer head received from
   * hello message
   */
  struct HeadMessage {
    std::vector<CID> blocks;    ///< heaviest tipset blocks cids
    primitives::BigInt weight;  ///< heaviest tipset weight
  };

  /** @brief BootstrapState enum reflects state of synchronization */
  enum class BootstrapState : int {
    STATE_INIT = 0,
    STATE_SYNCHRONIZING,
    STATE_COMPLETE,
  };

  using boost::asio::io_context;
  using libp2p::peer::PeerId;
  using network::sync::Sync;
  using network::sync::SyncState;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using storage::blockchain::ChainStore;
  using storage::ipfs::IpfsBlockService;
  using storage::ipfs::IpfsDatastore;

  class ChainSynchronizer;

  /** class SyncSyncManagerer manages chains downloading and keeping them
   * up-to-date */
  class SyncManager : public common::Stateful<BootstrapState> {
   public:
    virtual ~SyncManager() = default;

    virtual outcome::result<bool> onHeadReceived(const HeadMessage &msg) = 0;

    virtual std::shared_ptr<ChainSynchronizer> findRelatedChain(
        const TipsetKey &key) = 0;

    virtual void startSynchronizer(const TipsetKey &key) = 0;

    using SyncStateChangedFn = void(const SyncState &);

    boost::signals2::connection connectSyncStateChanged(SyncStateChangedFn cb) {
      return sync_state_changed_.connect(cb);
    }

   private:
    boost::signals2::signal<SyncStateChangedFn> sync_state_changed_;
  };
}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP
