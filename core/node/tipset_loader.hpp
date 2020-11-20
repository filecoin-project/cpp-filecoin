/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_TIPSET_LOADER_HPP
#define CPP_FILECOIN_SYNC_TIPSET_LOADER_HPP

#include "blocksync_client.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::sync {

  namespace events {
    struct BlockStored;
  }

  class ChainDb;

  class TipsetLoader : public std::enable_shared_from_this<TipsetLoader> {
   public:
    enum class Error {
      TIPSET_LOADER_NOT_INITIALIZED = 1,
      TIPSET_LOADER_NO_PEERS,
      TIPSET_LOADER_BAD_TIPSET
    };

    explicit TipsetLoader(
        std::shared_ptr<blocksync::BlocksyncClient> blocksync,
        std::shared_ptr<ChainDb> chain_db);

    void start(std::shared_ptr<events::Events> events);

    outcome::result<void> loadTipsetAsync(
        const TipsetKey &key,
        boost::optional<PeerId> preferred_peer,
        uint64_t depth = 1);

   private:
    void onBlock(const events::BlockStored& event);

    using Wantlist = std::unordered_set<CID>;

    struct RequestCtx {
      TipsetLoader &owner;

      TipsetKey tipset_key;

      // block cids we are waiting for
      Wantlist wantlist;

      // the puzzle being filled
      std::vector<boost::optional<BlockHeader>> blocks_filled;

      bool is_bad_tipset = false;

      RequestCtx(TipsetLoader &o, const TipsetKey &key);

      void onBlockSynced(const CID &cid, const BlockHeader &bh);

      void onError(const CID &cid, std::error_code error);
    };

    void onRequestCompleted(TipsetHash hash,
                            outcome::result<TipsetCPtr> result);

    std::shared_ptr<blocksync::BlocksyncClient> blocksync_;
    std::shared_ptr<ChainDb> chain_db_;
    std::shared_ptr<events::Events> events_;
    std::map<TipsetHash, RequestCtx> tipset_requests_;
    Wantlist global_wantlist_;
    events::Connection block_stored_event_;
    boost::optional<PeerId> last_peer_;
    bool initialized_ = false;

    // friendship with contained objects is ok
    friend RequestCtx;
  };

}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, TipsetLoader::Error);

#endif  // CPP_FILECOIN_SYNC_TIPSET_LOADER_HPP
