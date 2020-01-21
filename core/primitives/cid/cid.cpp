/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/cid.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include "crypto/blake2/blake2b160.hpp"

namespace fc {
  CID::CID()
      : ContentIdentifier(
          {}, {}, libp2p::multi::Multihash::create({}, {}).value()) {}

  CID::CID(const ContentIdentifier &cid) : ContentIdentifier(cid) {}

  outcome::result<std::string> CID::toString() const {
    return libp2p::multi::ContentIdentifierCodec::toString(*this);
  }
}  // namespace fc

namespace fc::common {
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes) {
    OUTCOME_TRY(hash_raw, crypto::blake2b::blake2b_256(bytes));
    OUTCOME_TRY(hash,
                libp2p::multi::Multihash::create(
                    libp2p::multi::HashType::blake2b_256, hash_raw));
    return CID(CID::Version::V1, libp2p::multi::MulticodecType::DAG_CBOR, hash);
  }
}  // namespace fc::common
