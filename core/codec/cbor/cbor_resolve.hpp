/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_decode_stream.hpp"

namespace fc::codec::cbor {
  enum class CborResolveError {
    kIntKeyExpected = 1,
    kKeyNotFound,
    kContainerExpected,
    kIntKeyTooBig
  };

  using Path = gsl::span<const std::string>;

  outcome::result<uint64_t> parseIndex(const std::string &str);

  /** Resolves path in CBOR object to CBOR subobject */
  outcome::result<void> resolve(CborDecodeStream &stream,
                                const std::string &part);
}  // namespace fc::codec::cbor

OUTCOME_HPP_DECLARE_ERROR(fc::codec::cbor, CborResolveError);
