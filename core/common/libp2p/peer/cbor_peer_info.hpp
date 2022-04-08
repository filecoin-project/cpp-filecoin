/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "common/libp2p/peer/cbor_peer_id.hpp"
#include "common/default_t.hpp"

namespace fc::common {
  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;

  template <>
  inline PeerInfo kDefaultT<PeerInfo>() {
    return PeerInfo{.id = kDefaultT<PeerId>(), .addresses = {}};
  }

  template <>
  inline std::tuple<PeerInfo> kDefaultT<std::tuple<PeerInfo>>() {
    return std::make_tuple(kDefaultT<PeerInfo>());
  }
}  // namespace fc::codec::cbor

namespace libp2p::peer {

  CBOR_TUPLE(PeerInfo, id, addresses);

}  // namespace libp2p::peer
