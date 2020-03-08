/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_RESOLVE_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_RESOLVE_HPP

#include "filecoin/codec/cbor/cbor_decode_stream.hpp"

namespace fc::codec::cbor {
  enum class CborResolveError {
    INT_KEY_EXPECTED = 1,
    KEY_NOT_FOUND,
    CONTAINER_EXPECTED,
    INT_KEY_TOO_BIG
  };

  using Path = std::vector<std::string>;

  /** Resolves path in CBOR object to CBOR subobject */
  outcome::result<std::pair<std::vector<uint8_t>, Path>> resolve(
      gsl::span<const uint8_t> node, const Path &path);
}  // namespace fc::codec::cbor

OUTCOME_HPP_DECLARE_ERROR(fc::codec::cbor, CborResolveError);

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_RESOLVE_HPP
