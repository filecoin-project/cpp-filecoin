/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_TIPSET_LOADER_HPP
#define CPP_FILECOIN_SYNC_TIPSET_LOADER_HPP

#include <set>

#include "common.hpp"
#include "block_loader.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::sync {

  class TipsetLoader {
   public:
    /// Called when all tipset subobjects are available
    /// or tipset appeared to be bad
    using OnTipset = std::function<void(TipsetHash hash,
                                        outcome::result<Tipset> tipset)>;

    TipsetLoader(std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                 std::shared_ptr<BlockLoader> block_loader);

    void init(OnTipset callback);

    /// Returns immediately if tipset is available locally, otherwise
    /// begins synchronizing its subobjects from the network
    outcome::result<boost::optional<Tipset>> loadTipset(
        const TipsetKey &key,
        boost::optional<std::reference_wrapper<const PeerId>> preferred_peer);

   private:
    void onBlock(const CID& cid, outcome::result<BlockHeader> bh);

    using Wantlist = std::set<CID>;

    struct RequestCtx {
      TipsetLoader &owner;

      TipsetKey tipset_key;

      // block cids we are waiting for
      Wantlist wantlist;

      // the puzzle being filled
      std::vector<boost::optional<BlockHeader>> blocks_filled;

      bool is_bad_tipset = false;

      libp2p::protocol::scheduler::Handle call_completed;

      RequestCtx(TipsetLoader &o, const TipsetKey &key);

      void onBlockSynced(const CID &cid, const BlockHeader &bh);

      void onError(const CID &cid);
    };

    void onRequestCompleted(TipsetHash hash,
        outcome::result<Tipset> tipset);

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<BlockLoader> block_loader_;
    OnTipset callback_;
    std::map<TipsetHash, RequestCtx> tipset_requests_;
    Wantlist global_wantlist_;
    bool initialized_ = false;

    // friendship with contained objects is ok
    friend RequestCtx;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_TIPSET_LOADER_HPP
