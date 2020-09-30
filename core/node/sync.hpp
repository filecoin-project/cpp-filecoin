/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "node/fwd.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::sync {
  using libp2p::Host;
  using libp2p::peer::PeerId;
  using primitives::block::BlockWithCids;
  using primitives::tipset::TipsetKey;
  using storage::blockchain::ChainStore;
  using vm::interpreter::Interpreter;

  struct TsSync : public std::enable_shared_from_this<TsSync> {
    using Callback = std::function<void(const TipsetKey &, bool)>;

    TsSync(std::shared_ptr<Host> host,
           IpldPtr ipld,
           std::shared_ptr<Interpreter> interpreter);
    void sync(const TipsetKey &key, const PeerId &peer, Callback callback);
    void walkDown(TipsetKey key, const PeerId &peer);
    void walkUp(TipsetKey key);

    std::shared_ptr<Host> host;
    IpldPtr ipld;
    std::shared_ptr<Interpreter> interpreter;
    std::unordered_map<TipsetKey, std::vector<Callback>> callbacks;
    std::unordered_map<TipsetKey, std::vector<TipsetKey>> children;

    // TODO: component, persistent/caching
    std::unordered_map<TipsetKey, bool> valid;
  };

  struct Sync : public std::enable_shared_from_this<Sync> {
    Sync(IpldPtr ipld,
         std::shared_ptr<TsSync> ts_sync,
         std::shared_ptr<ChainStore> chain_store);
    void onHello(const TipsetKey &key, const PeerId &peer);
    outcome::result<void> onGossip(const BlockWithCids &block,
                                   const PeerId &peer);

    IpldPtr ipld;
    std::shared_ptr<TsSync> ts_sync;
    std::shared_ptr<ChainStore> chain_store;
  };
}  // namespace fc::sync
