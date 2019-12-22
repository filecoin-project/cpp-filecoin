/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include "codec/cbor/cbor.hpp"
#include "crypto/blake2/blake2b160.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::hamt, HamtError, e) {
  using fc::storage::hamt::HamtError;
  switch (e) {
    case HamtError::EXPECTED_CID:
      return "Expected CID";
    default:
      return "Unknown error";
  }
}

namespace fc::storage::hamt {
  const CID kDummyCid({}, {}, libp2p::multi::Multihash::create({}, {}).value());

  outcome::result<CID> Node::cid(gsl::span<const uint8_t> encoded) {
    OUTCOME_TRY(hash_raw,
                crypto::blake2b::blake2b_256(encoded));
    OUTCOME_TRY(hash,
                libp2p::multi::Multihash::create(
                    libp2p::multi::HashType::blake2b_256, hash_raw));
    return CID(CID::Version::V1, libp2p::multi::MulticodecType::DAG_CBOR, hash);
  }

  outcome::result<CID> Node::cid() const {
    OUTCOME_TRY(encoded, codec::cbor::encode(*this));
    return cid(encoded);
  }
}  // namespace fc::storage::hamt
