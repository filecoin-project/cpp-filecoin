/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/crypto/vrf/vrf_hash_encoder.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include "filecoin/common/le_encoder.hpp"
#include "filecoin/primitives/address/address_codec.hpp"

namespace fc::crypto::vrf {
  using primitives::address::Protocol;

  outcome::result<VRFHash> encodeVrfParams(const VRFParams &params) {
    if (params.miner_address.getProtocol() != Protocol::BLS) {
      return VRFError::ADDRESS_IS_NOT_BLS;
    }
    auto &&miner_bytes = primitives::address::encode(params.miner_address);

    common::Buffer out{};
    auto required_bytes = sizeof(uint64_t) + 2 * sizeof(uint8_t)
                          + params.message.size() + miner_bytes.size();
    out.reserve(required_bytes);
    common::encodeLebInteger(static_cast<uint64_t>(params.personalization_tag),
                             out);
    out.putUint8(0u);
    out.putBuffer(params.message);
    out.putUint8(0u);
    out.put(miner_bytes);

    auto hash = libp2p::crypto::sha256(out);

    return VRFHash::fromSpan(hash);
  }

}  // namespace fc::crypto::vrf
