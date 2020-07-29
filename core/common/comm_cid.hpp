/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_COMM_CID_HPP
#define CPP_FILECOIN_CORE_COMMON_COMM_CID_HPP

#include <unordered_map>
#include "common/blob.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::common {
  // kCommitmentBytesLen is the number of bytes in a CommR, CommD, CommP, and
  // CommRStar.
  const int kCommitmentBytesLen = 32;
  using Comm = Blob<kCommitmentBytesLen>;

  using FilMultiCodec = CID::Multicodec;
  using FilMultiHash = libp2p::multi::HashType;

  outcome::result<CID> replicaCommitmentV1ToCID(
      gsl::span<const uint8_t> comm_r);
  outcome::result<CID> dataCommitmentV1ToCID(gsl::span<const uint8_t> comm_d);
  outcome::result<CID> pieceCommitmentV1ToCID(gsl::span<const uint8_t> comm_p);

  outcome::result<Comm> CIDToPieceCommitmentV1(const CID &cid);
  outcome::result<Comm> CIDToReplicaCommitmentV1(const CID &cid);
  outcome::result<Comm> CIDToDataCommitmentV1(const CID &cid);

  enum class CommCidErrors {
    kIncorrectCodec = 1,
    kIncorrectHash,
    kInvalidCommSize,
  };
};  // namespace fc::common

OUTCOME_HPP_DECLARE_ERROR(fc::common, CommCidErrors);

#endif  // CPP_FILECOIN_CORE_COMMON_COMM_CID_HPP
