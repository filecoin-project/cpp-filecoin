/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/comm_cid.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/comm_cid_errors.hpp"

namespace fc::common {

  using libp2p::multi::HashType;

  CID replicaCommitmentV1ToCID(gsl::span<const uint8_t> comm_r) {
    OUTCOME_EXCEPT(cid, commitmentToCID(comm_r, FC_SEALED_V1));
    return cid;
  }

  CID dataCommitmentV1ToCID(gsl::span<const uint8_t> comm_d) {
    OUTCOME_EXCEPT(cid, commitmentToCID(comm_d, FC_UNSEALED_V1));
    return cid;
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
      return CommCidError::kInvalidHash;
    }

    OUTCOME_TRY(mh, Multihash::create(static_cast<HashType>(code), commitment));

    return CID(
        libp2p::multi::ContentIdentifier::Version::V1, kFilecoinCodecType, mh);
  }

  outcome::result<Comm> CIDToPieceCommitmentV1(const CID &cid) {
    return CIDToDataCommitmentV1(cid);
  }

  outcome::result<Comm> CIDToDataCommitmentV1(const CID &cid) {
    OUTCOME_TRY(result, CIDToCommitment(cid));
    if (static_cast<FilecoinHashType>(result.getType()) != FC_UNSEALED_V1) {
      return CommCidError::kInvalidHash;
    }
    return Comm::fromSpan(result.getHash());
  }

  outcome::result<Multihash> CIDToCommitment(const CID &cid) {
    if (!validFilecoinMultihash(cid.content_address.getType())) {
      return CommCidError::kInvalidHash;
    }
    return cid.content_address;
  }

  outcome::result<Comm> CIDToReplicaCommitmentV1(const CID &cid) {
    OUTCOME_TRY(result, CIDToCommitment(cid));
    if (static_cast<FilecoinHashType>(result.getType()) != FC_SEALED_V1) {
      return CommCidError::kInvalidHash;
    }
    return Comm::fromSpan(result.getHash());
  }
}  // namespace fc::common
