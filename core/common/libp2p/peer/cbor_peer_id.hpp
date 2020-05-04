/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_CBOR_PEER_ID_HPP
#define CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_CBOR_PEER_ID_HPP

#include <libp2p/peer/peer_id.hpp>
#include "codec/cbor/streams_annotation.hpp"

using libp2p::multi::Multihash;
using libp2p::peer::PeerId;

/**
 * Default value of PeerId for CBOR stream decoder
 */
template <>
inline const PeerId fc::codec::cbor::kDefaultT<PeerId>{
    PeerId::fromHash(
        Multihash::create(libp2p::multi::sha256, Buffer(43, 1)).value())
        .value()};

namespace libp2p::peer {

  CBOR_ENCODE(PeerId, peer) {
    return s << peer.toVector();
  }

  CBOR_DECODE(PeerId, peer) {
    std::vector<uint8_t> bytes;
    s >> bytes;
    OUTCOME_EXCEPT(peer_id, PeerId::fromBytes(bytes));
    peer = std::move(peer_id);
    return s;
  }

}  // namespace libp2p::peer

#endif  // CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_CBOR_PEER_ID_HPP
