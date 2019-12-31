/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrfhash_provider.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include "common/le_encoder.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::crypto::vrf {
  using primitives::address::Protocol;

  outcome::result<VRFHash> VRFHashProvider::create(DomainSeparationTag tag,
                                                   const Address &miner_address,
                                                   const Buffer &message) {
    if (miner_address.getProtocol() != Protocol::BLS) {
      return VRFError::ADDRESS_IS_NOT_BLS;
    }
    auto &&miner_bytes = primitives::address::encode(miner_address);

    Buffer out{};
    auto required_bytes = sizeof(uint64_t) + 2 * sizeof(uint8_t)
                          + message.size() + miner_bytes.size();
    out.reserve(required_bytes);
    common::encodeInteger(static_cast<uint64_t>(tag), out);
    out.putUint8(0u);
    out.putBuffer(message);
    out.putUint8(0u);
    out.put(miner_bytes);
    auto hash = libp2p::crypto::sha256(out);

    return common::Hash256::fromSpan(hash);
  }
}  // namespace fc::crypto::vrf
