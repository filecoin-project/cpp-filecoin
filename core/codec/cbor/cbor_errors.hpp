/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ERRORS_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ERRORS_HPP

#include "common/outcome.hpp"

namespace fc::codec::cbor {
  enum class CborEncodeError { kInvalidCID = 1, kExpectedMapValueSingle };

  enum class CborDecodeError {
    kInvalidCbor = 1,
    kWrongType,
    kIntOverflow,
    kInvalidCborCID,
    kInvalidCID,
    kWrongSize,
  };
}  // namespace fc::codec::cbor

OUTCOME_HPP_DECLARE_ERROR(fc::codec::cbor, CborEncodeError);
OUTCOME_HPP_DECLARE_ERROR(fc::codec::cbor, CborDecodeError);

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ERRORS_HPP
