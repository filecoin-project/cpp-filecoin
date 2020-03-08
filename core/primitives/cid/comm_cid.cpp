/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/cid/comm_cid.hpp"
#include <libp2p/multi/uvarint.hpp>
#include "filecoin/primitives/cid/comm_cid_errors.hpp"

namespace fc::common {

  Comm cppCommitment(gsl::span<const uint8_t> bytes) {
    if (bytes.size() != 32) return {};
    Comm result;
    std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
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
    return kFilecoinMultihashNames.find(code) != kFilecoinMultihashNames.end();
  }

  outcome::result<CID> commitmentToCID(gsl::span<const uint8_t> commitment,
                                       FilecoinMultihashCode code) {
    if (!validFilecoinMultihash(code)) {
      return CommCidError::INVALID_HASH;
    }

    using Version = libp2p::multi::ContentIdentifier::Version;

    OUTCOME_TRY(mh, Multihash::create((HashType)code, commitment));

    return CID(Version::V1, kFilecoinCodecType, mh);
  }

  outcome::result<Comm> CIDToPieceCommitmentV1(const CID &cid) {
    return CIDToDataCommitmentV1(cid);
  }

  outcome::result<Comm> CIDToDataCommitmentV1(const CID &cid) {
    OUTCOME_TRY(result, CIDToCommitment(cid));
    if ((FilecoinHashType)result.getType() != FC_UNSEALED_V1) {
      return CommCidError::INVALID_HASH;
    }
    return cppCommitment(result.getHash());
  }

  outcome::result<Multihash> CIDToCommitment(const CID &cid) {
    OUTCOME_TRY(result,
                Multihash::createFromBytes(cid.content_address.toBuffer()));

    if (!validFilecoinMultihash(result.getType())) {
      return CommCidError::INVALID_HASH;
    }

    return std::move(result);
  }

    outcome::result<Comm> CIDToReplicaCommitmentV1(const CID &cid) {
        OUTCOME_TRY(result, CIDToCommitment(cid));
        if ((FilecoinHashType)result.getType() != FC_SEALED_V1) {
            return CommCidError::INVALID_HASH;
        }
        return cppCommitment(result.getHash());
    }
}  // namespace fc::common
