/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "comm_cid.hpp"
#include "common/outcome.hpp"

namespace fc::common {

  namespace {

    outcome::result<void> validateFilCIDSegments(
        FilMultiCodec codec,
        FilMultiHash hash,
        gsl::span<const uint8_t> comm_x) {
      switch (codec) {
        case FilMultiCodec::FILECOIN_COMMITMENT_UNSEALED:
          if (hash != FilMultiHash::sha2_256_trunc254_padded) {
            return CommCidErrors::kIncorrectHash;
          }
          break;
        case FilMultiCodec::FILECOIN_COMMITMENT_SEALED:
          if (hash != FilMultiHash::poseidon_bls12_381_a1_fc1) {
            return CommCidErrors::kIncorrectHash;
          }
          break;
        default:
          return CommCidErrors::kIncorrectCodec;
      }

      if (comm_x.size() != common::kCommitmentBytesLen) {
        return CommCidErrors::kInvalidCommSize;
      }

      return outcome::success();
    }

    outcome::result<CID> commitmentToCID(FilMultiCodec codec,
                                         FilMultiHash hash,
                                         gsl::span<const uint8_t> comm_x) {
      OUTCOME_TRY(validateFilCIDSegments(codec, hash, comm_x));

      OUTCOME_TRY(mh, libp2p::multi::Multihash::create(hash, comm_x));

      return CID(libp2p::multi::ContentIdentifier::Version::V1, codec, mh);
    }

    outcome::result<Comm> CIDToCommitment(const CID &cid,
                                          FilMultiCodec expected_codec) {
      if (cid.content_type != expected_codec) {
        return CommCidErrors::kIncorrectCodec;
      }

      OUTCOME_TRY(validateFilCIDSegments(cid.content_type,
                                         cid.content_address.getType(),
                                         cid.content_address.getHash()));

      return Comm::fromSpan(cid.content_address.getHash());
    }
  }  // namespace

  outcome::result<CID> replicaCommitmentV1ToCID(
      gsl::span<const uint8_t> comm_r) {
    return commitmentToCID(FilMultiCodec::FILECOIN_COMMITMENT_SEALED,
                           FilMultiHash::poseidon_bls12_381_a1_fc1,
                           comm_r);
  }

  outcome::result<CID> dataCommitmentV1ToCID(gsl::span<const uint8_t> comm_d) {
    return commitmentToCID(FilMultiCodec::FILECOIN_COMMITMENT_UNSEALED,
                           FilMultiHash::sha2_256_trunc254_padded,
                           comm_d);
  }

  outcome::result<CID> pieceCommitmentV1ToCID(gsl::span<const uint8_t> comm_p) {
    return dataCommitmentV1ToCID(comm_p);
  }

  outcome::result<Comm> CIDToPieceCommitmentV1(const CID &cid) {
    return CIDToDataCommitmentV1(cid);
  }

  outcome::result<Comm> CIDToDataCommitmentV1(const CID &cid) {
    return CIDToCommitment(cid, FilMultiCodec::FILECOIN_COMMITMENT_UNSEALED);
  }

  outcome::result<Comm> CIDToReplicaCommitmentV1(const CID &cid) {
    return CIDToCommitment(cid, FilMultiCodec::FILECOIN_COMMITMENT_SEALED);
  }
}  // namespace fc::common

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, CommCidErrors, e) {
  using fc::common::CommCidErrors;

  switch (e) {
    case (CommCidErrors::kIncorrectCodec):
      return "CommCid: unexpected commitment codec";
    case (CommCidErrors::kIncorrectHash):
      return "CommCid: incorrect hashing function for data commitment";
    case (CommCidErrors::kInvalidCommSize):
      return "CommCid: commitments must be 32 bytes long";
    default:
      return "CommCid: unknown error";
  }
}
