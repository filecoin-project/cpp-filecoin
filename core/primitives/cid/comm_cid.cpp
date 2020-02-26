/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/comm_cid.hpp"
#include <libp2p/multi/uvarint.hpp>
#include "primitives/cid/comm_cid_errors.hpp"

namespace fc::common {

  outcome::result<Multihash> rawMultiHash(FilecoinMultihashCode code,
                                          gsl::span<const uint8_t> commitment) {
    UVarint u_int(code);
    UVarint u_int_len(commitment.size());
    auto bytes = u_int.toVector();
    bytes.insert(bytes.end(), u_int_len.toVector().begin(), u_int_len.toVector().end());
    bytes.insert(bytes.end(), commitment.begin(), commitment.end());

    return Multihash::createFromBytes(bytes);
  }

  outcome::result<std::pair<FilecoinMultihashCode, gsl::span<const uint8_t>>>
  readMultiHashFromBuf(gsl::span<const uint8_t> bytes) {
    auto bufl = bytes.size();
    if (bufl < 2) {
      return CommCidError::TOO_SHORT;
    }

    auto code_opt = UVarint::create(bytes);
    if (!code_opt.has_value()) {
      return CommCidError::CANT_READ_CODE;
    }
    auto code = code_opt.value();

    auto other_bytes =
        gsl::make_span(bytes.data() + code.size(), bufl - code.size());
    auto length_opt = UVarint::create(other_bytes);
    if (!length_opt.has_value()) {
      return CommCidError::CANT_READ_LENGTH;
    }
    auto length = length_opt.value();

    if (std::numeric_limits<uint32_t>::max() < length.toUInt64()) {
      return CommCidError::DATA_TOO_BIG;
    }
    if (other_bytes.size() - length.size() != length.toUInt64()) {
      return CommCidError::DIFFERENT_LENGTH;
    }

    return std::make_pair(
        code.toUInt64(),
        gsl::make_span(other_bytes.data() + length.size(), length.toUInt64()));
  }

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

    OUTCOME_TRY(mh, rawMultiHash(code, commitment));

    return CID(Version::V1, kFilecoinCodecType, mh);
  }

  outcome::result<Comm> CIDToPieceCommitmentV1(const CID &cid) {
    return CIDToDataCommitmentV1(cid);
  }

  outcome::result<Comm> CIDToDataCommitmentV1(const CID &cid) {
    OUTCOME_TRY(res, CIDToCommitment(cid));
    if (res.first != FC_UNSEALED_V1) {
      return CommCidError::INVALID_HASH;
    }
    return cppCommitment(res.second);
  }

  outcome::result<std::pair<FilecoinMultihashCode, gsl::span<const uint8_t>>>
  CIDToCommitment(const CID &cid) {
    OUTCOME_TRY(result, readMultiHashFromBuf(cid.content_address.toBuffer()));

    if (!validFilecoinMultihash(result.first)) {
      return CommCidError::INVALID_HASH;
    }

    return std::move(result);
  }
}  // namespace fc::common
