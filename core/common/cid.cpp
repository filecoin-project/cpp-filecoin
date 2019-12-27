/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/cid.hpp"

#include "crypto/blake2/blake2b160.hpp"

namespace fc::common {
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes) {
    OUTCOME_TRY(hash_raw, crypto::blake2b::blake2b_256(bytes));
    OUTCOME_TRY(hash,
                libp2p::multi::Multihash::create(
                    libp2p::multi::HashType::blake2b_256, hash_raw));
    return CID(CID::Version::V1, libp2p::multi::MulticodecType::DAG_CBOR, hash);
  }
}  // namespace fc::common
