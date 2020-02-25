/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/comm_cid.hpp"
#include <libp2p/multi/uvarint.hpp>

namespace fc::common {

  outcome::result<libp2p::multi::Multihash> rawMultiHash(
      FilecoinMultihashCode code, gsl::span<const uint8_t> commitment) {
    libp2p::multi::UVarint u_int(code);
    libp2p::multi::UVarint u_int_len(commitment.size());
    auto bytes = u_int.toVector();
    bytes.insert(bytes.end(), u_int.toVector().begin(), u_int.toVector().end());
    bytes.insert(bytes.end(), commitment.begin(), commitment.end());

    return libp2p::multi::Multihash::createFromBytes(bytes);
  }

  CID replicaCommitmentV1ToCID(gsl::span<const uint8_t> comm_r) {
    auto cid = commitmentToCID(comm_r, FilecoinHashType::FC_SEALED_V1);
    if (cid.has_error()) return CID();
    return cid.value();
  }

  CID dataCommitmentV1ToCID(gsl::span<const uint8_t> comm_d) {
    auto cid = commitmentToCID(comm_d, FC_UNSEALED_V1);
    if (cid.has_error()) return CID();
    return cid.value();
  }

  CID pieceCommitmentV1ToCID(gsl::span<const uint8_t> comm_p) {
    return dataCommitmentV1ToCID(comm_p);
  }

  bool validFilecoinMultihash(FilecoinMultihashCode code) {
    return kFilecoinMultihashNames.find(code) == kFilecoinMultihashNames.end();
  }

  outcome::result<CID> commitmentToCID(gsl::span<const uint8_t> commitment,
                                       FilecoinMultihashCode code) {
    if (!validFilecoinMultihash(code)) {
      return outcome::success();  // ERROR
    }

    using Version = libp2p::multi::ContentIdentifier::Version;

    OUTCOME_TRY(mh, rawMultiHash(code, commitment));

    return CID(Version::V1, kFilecoinCodecType, mh);
  }
}  // namespace fc::common
