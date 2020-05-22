/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_CBOR_PEER_INFO_HPP
#define CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_CBOR_PEER_INFO_HPP

#include <libp2p/peer/peer_id.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "common/libp2p/peer/cbor_peer_id.hpp"

using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;

namespace fc::codec::cbor {
  template <>
  inline PeerInfo kDefaultT<PeerInfo>() {
    return PeerInfo{.id = kDefaultT<PeerId>(), .addresses = {}};
  }
}  // namespace fc::codec::cbor

namespace libp2p::peer {

  CBOR_TUPLE(PeerInfo, id, addresses);

}  // namespace libp2p::peer

#endif  // CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_CBOR_PEER_INFO_HPP
