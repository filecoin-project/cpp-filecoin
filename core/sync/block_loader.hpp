/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCK_LOADER_HPP
#define CPP_FILECOIN_SYNC_BLOCK_LOADER_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include "storage/ipfs/datastore.hpp"
#include "blocksync_client.hpp"

namespace fc::sync {

  class BlockLoader {
   public:
    /// Called when all tipset subobjects are available
    /// or tipset appeared to be bad
    using OnBlockSynced = std::function<void(
        const CID &block_cid, outcome::result<BlockHeader> header)>;

    using BlocksAvailable = std::vector<boost::optional<BlockHeader>>;

    BlockLoader(std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
                std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                std::shared_ptr<blocksync::BlocksyncClient> blocksync);

    void init(OnBlockSynced callback);

    outcome::result<BlocksAvailable> loadBlocks(
        const std::vector<CID> &cids,
        boost::optional<PeerId> preferred_peer,
        uint64_t load_parents_depth = 1);

   private:
    // Block and all its subobjects are in local storage if result has value,
    // otherwise error
    void onBlockStored(CID block_cid, outcome::result<BlockMsg> result);

    bool onBlockHeader(const CID &cid,
                       boost::optional<BlockHeader> header,
                       bool block_completed);

    using Wantlist = std::set<CID>;

    struct RequestCtx {
      BlockLoader &owner;
      CID block_cid;
      boost::optional<BlockHeader> header;
      bool is_bad = false;

      libp2p::protocol::scheduler::Handle call_completed;

      RequestCtx(BlockLoader &o, const CID &cid);

      void onBlockHeader(boost::optional<BlockHeader> bh, bool block_completed);
    };

    struct BlockAvailable {
      boost::optional<BlockHeader> header;
      bool all_messages_available = false;
      bool meta_available = false;
      Wantlist bls_messages_to_load;
      Wantlist secp_messages_to_load;
    };

    outcome::result<boost::optional<BlockHeader>> tryLoadBlock(const CID &cid);

    outcome::result<BlockAvailable> findBlockInLocalStore(const CID &cid);

    void onRequestCompleted(CID block_cid,
                            boost::optional<BlockHeader> bh);

    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<blocksync::BlocksyncClient> blocksync_;
    OnBlockSynced callback_;
    bool initialized_ = false;

    using RequestCtxPtr = std::shared_ptr<RequestCtx>;

    std::map<CID, RequestCtxPtr> block_requests_;
    std::vector<CID> wanted_;

    // friendship with contained objects is ok
    friend RequestCtx;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_BLOCK_LOADER_HPP
