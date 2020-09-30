/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/fwd.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::hello {
  using libp2p::Host;
  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;
  using primitives::BigInt;
  using storage::blockchain::ChainStore;

  struct Hello {
    struct State {
      std::vector<CID> blocks;
      uint64_t height;
      BigInt weight;
      CID genesis;
    };

    struct Latency {
      int64_t arrival{0}, sent{0};
    };

    using StateCb = std::function<void(const PeerId &, State)>;

    Hello(std::shared_ptr<Host> host,
          std::shared_ptr<ChainStore> chain_store,
          StateCb state_cb);
    void say(const PeerInfo &peer);

    std::shared_ptr<Host> host;
    std::shared_ptr<ChainStore> chain_store;
  };
  CBOR_TUPLE(Hello::State, blocks, height, weight, genesis)
  CBOR_TUPLE(Hello::Latency, arrival, sent)
}  // namespace fc::hello
