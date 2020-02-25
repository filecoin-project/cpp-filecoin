/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_COMM_CID_HPP
#define CPP_FILECOIN_COMM_CID_HPP

#include <unordered_map>
#include "primitives/cid/cid.hpp"

namespace fc::common {
  CID replicaCommitmentV1ToCID(gsl::span<const uint8_t> comm_r);
  CID dataCommitmentV1ToCID(gsl::span<const uint8_t> comm_d);
  CID pieceCommitmentV1ToCID(gsl::span<const uint8_t> comm_p);

  using FilecoinMultihashCode = uint64_t;

  enum FilecoinHashType : FilecoinMultihashCode {
    // FC_UNSEALED_V1 is the v1 hashing algorithm used in
    // constructing merkleproofs of unsealed data
    FC_UNSEALED_V1 = 0xfc1,
    // FC_SEALED_V1 is the v1 hashing algorithm used in
    // constructing merkleproofs of sealed replicated data
    FC_SEALED_V1,

    // FC_RESERVED3 is reserved for future use
    FC_RESERVED3,

    // FC_RESERVED4 is reserved for future use
    FC_RESERVED4,

    // FC_RESERVED5 is reserved for future use
    FC_RESERVED5,

    // FC_RESERVED6 is reserved for future use
    FC_RESERVED6,

    // FC_RESERVED7 is reserved for future use
    FC_RESERVED7,

    // FC_RESERVED8 is reserved for future use
    FC_RESERVED8,

    // FC_RESERVED9 is reserved for future use
    FC_RESERVED9,

    // FC_RESERVED10 is reserved for future use
    FC_RESERVED10,
  };

  const std::unordered_map<FilecoinMultihashCode, const std::string>
      kFilecoinMultihashNames({
          {FC_UNSEALED_V1, "Filecoin Merkleproof Of Unsealed Data, V1"},
          {FC_SEALED_V1, "Filecoin Merkleproof Of Sealed Data, V1"},
          {FC_RESERVED3, "Reserved"},
          {FC_RESERVED4, "Reserved"},
          {FC_RESERVED5, "Reserved"},
          {FC_RESERVED6, "Reserved"},
          {FC_RESERVED7, "Reserved"},
          {FC_RESERVED8, "Reserved"},
          {FC_RESERVED9, "Reserved"},
          {FC_RESERVED10, "Reserved"},
      });

  const auto kFilecoinCodecType = libp2p::multi::MulticodecType::Code::RAW;

  outcome::result<CID> commitmentToCID(gsl::span<const uint8_t> commitment,
                                       FilecoinMultihashCode code);
};  // namespace fc::common

#endif  // CPP_FILECOIN_COMM_CID_HPP
