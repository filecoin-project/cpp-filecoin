/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::cbor, CborEncodeError, e) {
  using fc::codec::cbor::CborEncodeError;
  switch (e) {
    case CborEncodeError::kInvalidCID:
      return "Invalid CID";
    case CborEncodeError::kExpectedMapValueSingle:
      return "Expected map value single";
    default:
      return "Unknown error";
  }
}

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::cbor, CborDecodeError, e) {
  using fc::codec::cbor::CborDecodeError;
  switch (e) {
    case CborDecodeError::kInvalidCbor:
      return "Invalid CBOR";
    case CborDecodeError::kWrongType:
      return "Wrong type";
    case CborDecodeError::kIntOverflow:
      return "Int overflow";
    case CborDecodeError::kInvalidCborCID:
      return "Invalid CBOR CID";
    case CborDecodeError::kInvalidCID:
      return "Invalid CID";
    case CborDecodeError::kWrongSize:
      return "Wrong size";
    default:
      return "Unknown error";
  }
}
