/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "codec/json/coding.hpp"

namespace fc::codec::json {
  using libp2p::peer::PeerId;

  /// Default value of PeerId for JSON decoder
  template <>
  inline PeerId kDefaultT<PeerId>() {
    return PeerId::fromHash(
               libp2p::multi::Multihash::create(libp2p::multi::sha256, {})
                   .value())
        .value();
  }
}  // namespace fc::codec::json

namespace libp2p::peer {

  JSON_ENCODE(PeerId) {
    return fc::codec::json::encode( v.toBase58(), allocator);
  }

  JSON_DECODE(PeerId) {
    OUTCOME_EXCEPT(id, PeerId::fromBase58(fc::codec::json::AsString(j)));
    v = std::move(id);
  }

}  // namespace libp2p::peer
