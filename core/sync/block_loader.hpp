/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCK_LOADER_HPP
#define CPP_FILECOIN_SYNC_BLOCK_LOADER_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include "common.hpp"
#include "object_loader.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs::graphsync {
  class Graphsync;
}

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
                std::shared_ptr<ObjectLoader> object_loader);

    void init(OnBlockSynced callback);

    outcome::result<BlocksAvailable> loadBlocks(
        const std::vector<CID> &cids,
        boost::optional<std::reference_wrapper<const PeerId>> preferred_peer);

   private:
    bool onBlockHeader(const CID &cid,
                       bool object_is_valid,
                       boost::optional<BlockHeader> header);

    bool onMsgMeta(const CID &cid,
                   bool object_is_valid,
                   const std::vector<CID> &bls_messages,
                   const std::vector<CID> &secp_messages);

    bool onMessage(const CID &cid, bool is_secp_message, bool valid);

    using Wantlist = std::set<CID>;

    struct RequestCtx {
      BlockLoader &owner;
      CID block_cid;
      boost::optional<BlockHeader> header;
      Wantlist bls_messages;
      Wantlist secp_messages;
      bool is_bad = false;

      libp2p::protocol::scheduler::Handle call_completed;

      RequestCtx(BlockLoader &o, const CID &cid);

      void onBlockHeader(bool object_is_valid, boost::optional<BlockHeader> bh);

      void onMeta(bool object_is_valid, bool no_more_messages);

      void onMessage(const CID &cid, bool is_secp, bool object_is_valid);
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
    std::shared_ptr<ObjectLoader> object_loader_;
    OnBlockSynced callback_;
    bool initialized_ = false;

    using RequestCtxPtr = std::shared_ptr<RequestCtx>;

    std::map<CID, RequestCtxPtr> block_requests_;
    std::multimap<CID, RequestCtxPtr> meta_requests_;
    std::multimap<CID, RequestCtxPtr> msg_requests_;

    std::vector<ObjectLoader::ObjectWanted> wanted_;

    // friendship with contained objects is ok
    friend RequestCtx;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_BLOCK_LOADER_HPP
