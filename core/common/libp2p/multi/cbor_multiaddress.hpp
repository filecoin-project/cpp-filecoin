/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"

using libp2p::multi::Multiaddress;

namespace fc::codec::cbor {
  /// Default value of Multiaddress for CBOR stream decoder
  template <>
  inline Multiaddress kDefaultT<Multiaddress>() {
    return Multiaddress::create("/ip4/0.0.0.1/udp/1").value();
  };
}  // namespace fc::codec::cbor

namespace libp2p::multi {

  CBOR_ENCODE(Multiaddress, ma) {
    return s << ma.getBytesAddress();
  }

  CBOR_DECODE(Multiaddress, ma) {
    std::vector<uint8_t> bytes;
    s >> bytes;
    OUTCOME_EXCEPT(created, Multiaddress::create(bytes));
    ma = std::move(created);
    return s;
  }

}  // namespace libp2p::multi
