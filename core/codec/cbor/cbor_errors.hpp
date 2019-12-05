/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ERRORS_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ERRORS_HPP

#include "common/outcome.hpp"

namespace fc::codec::cbor {
  enum class CborEncodeError { INVALID_CID = 1, EXPECTED_MAP_VALUE_SINGLE };

  enum class CborDecodeError { INVALID_CBOR = 1, WRONG_TYPE, INT_OVERFLOW, INVALID_CBOR_CID, INVALID_CID };
}  // namespace fc::codec::cbor

OUTCOME_HPP_DECLARE_ERROR(fc::codec::cbor, CborEncodeError);
OUTCOME_HPP_DECLARE_ERROR(fc::codec::cbor, CborDecodeError);

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ERRORS_HPP
