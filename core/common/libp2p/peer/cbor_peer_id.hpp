/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "common/default_t.hpp"
#include "common/outcome.hpp"

namespace fc::common {
  using libp2p::peer::PeerId;

  /// Default value of PeerId for CBOR stream decoder
  template <>
  inline PeerId kDefaultT<PeerId>() {
    return PeerId::fromHash(
               libp2p::multi::Multihash::create(libp2p::multi::sha256, {})
                   .value())
        .value();
  }
}  // namespace fc::codec::cbor

namespace libp2p::peer {

  CBOR_ENCODE(PeerId, peer) {
    return s << peer.toVector();
  }

  CBOR_DECODE(PeerId, peer) {
    OUTCOME_EXCEPT(peer_id,
                   PeerId::fromBytes(s.template get<std::vector<uint8_t>>()));
    peer = std::move(peer_id);
    return s;
  }

}  // namespace libp2p::peer
