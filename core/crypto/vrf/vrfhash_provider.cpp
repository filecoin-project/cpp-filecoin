/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrfhash_provider.hpp"

#include <libp2p/multi/multihash.hpp>
#include "common/le_encoder.hpp"
#include "crypto/vrf/vrf_types.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::crypto::vrf {

  using libp2p::multi::HashType;
  using libp2p::multi::Multihash;

  outcome::result<VRFHash> VRFHashProvider::create(DomainSeparationTag tag,
                                                   const Address &miner_address,
                                                   const Buffer &message) {
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
    OUTCOME_TRY(multi_hash, Multihash::create(HashType::sha256, out));

    return common::Hash256::fromSpan(multi_hash.toBuffer());
  }
}  // namespace fc::crypto::vrf
