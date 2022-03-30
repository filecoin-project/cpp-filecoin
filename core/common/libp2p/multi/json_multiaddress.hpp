/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>

#include "codec/json/coding.hpp"

namespace fc::codec::json {
  using libp2p::multi::Multiaddress;

  /// Default value of Multiaddress for JSON decoder
  template <>
  inline Multiaddress kDefaultT<Multiaddress>() {
    return Multiaddress::create("/ip4/0.0.0.1/udp/1").value();
  };
}  // namespace fc::codec::json

namespace libp2p::multi {

  JSON_ENCODE(Multiaddress) {
    return fc::codec::json::encode(v.getStringAddress(), allocator);
  }

  JSON_DECODE(Multiaddress) {
    OUTCOME_EXCEPT(_v, Multiaddress::create(fc::codec::json::AsString(j)));
    v = std::move(_v);
  }

}  // namespace libp2p::multi
