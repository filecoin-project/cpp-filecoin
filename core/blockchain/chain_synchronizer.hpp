/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CHAIN_SYNCHRONIZER_HPP
#define CPP_FILECOIN_CHAIN_SYNCHRONIZER_HPP

#include <deque>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio/io_context.hpp>
#include "blockchain/sync_manager.hpp"
#include "common/logger.hpp"
#include "common/stateful.hpp"
#include "network/sync/sync.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/ipfs/batch.hpp"

namespace fc::blockchain {
  using boost::asio::io_context;
  using network::sync::Sync;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using storage::blockchain::ChainStore;
  using storage::ipfs::Batch;
  using storage::ipfs::IpfsDatastore;

  enum ChainSynchronizerState {
    STATE_INIT = 0,
    STATE_LOADING,
    STATE_CANCELED,
    STATE_COMPLETE,
  };

  /** @brief received object type*/
  enum class ObjectType {
    BAD = 0,
    MISSING,
    BLOCK,
    PERSISTENT,
    SIGNED_MESSAGE,
    UNSIGNED_MESSAGE,
  };

  /**
   * @class ChainSynchronizer downloads blocks and messages from network
   */
  class ChainSynchronizer : public common::Stateful<ChainSynchronizerState> {
   public:
    using LoadTicket = Sync::LoadTicket;

    explicit ChainSynchronizer(io_context &context,
                               std::weak_ptr<SyncManager> sync_manager,
                               std::shared_ptr<Sync> sync,
                               std::shared_ptr<ChainStore> store);

    /** @brief starts chain synchronization */
    void start(const TipsetKey &head, uint64_t limit);

    /** @brief stops synchronization */
    void cancel();

   private:
    /** @brief cancels all downloads of current chain loader */
    void stopDownloading();

    /** @brief schedules downloading of a block */
    void loadBlock(const CID &cid);

    /** @brief schedules downloading of a signed message */
    void loadSigMessage(const CID &cid);

    /** @brief schedules downloading of an unsigned message */
    void loadUnsigMessage(const CID &cid);

    /** @brief merges with another chain */
    outcome::result<void> merge(ChainSynchronizer &other);

    /** @brief handles received object */
    void onObjectReceived(const Sync::LoadResult &r);

    /** @brief checks current state and schedules next steps */
    void updateState();

    /** @brief checks object type and validity */
    ObjectType findObject(LoadTicket t, const CID &cid) const;

    /** @brief makes another attempt to download an object, which failed */
    void retryLoad(LoadTicket t);

    uint64_t limit_;  ///< number of chain items to download
    io_context &context_;
    std::weak_ptr<SyncManager> sync_manager_;
    std::shared_ptr<Sync> sync_;
    std::shared_ptr<ChainStore> chain_store_;

    std::deque<TipsetKey> chain_;
    std::unordered_map<LoadTicket, CID> block_tickets_;
    std::unordered_map<LoadTicket, CID> signed_msg_tickets_;
    std::unordered_map<LoadTicket, CID> unsigned_msg_tickets_;
    std::unordered_set<CID> blocks_;
    std::unordered_set<CID> signed_msgs_;
    std::unordered_set<CID> unsigned_msgs_;
    std::shared_ptr<Batch> blocks_batch_;

    std::vector<BlockHeader> current_headers_;  ///< current tipset

    common::Logger logger_;
  };
}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CHAIN_SYNCHRONIZER_HPP
