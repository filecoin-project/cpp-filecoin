/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/rle/rle_plus_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::rle, RLEPlusDecodeError, e) {
  using fc::codec::rle::RLEPlusDecodeError;
  switch (e) {
    case (RLEPlusDecodeError::kVersionMismatch):
      return "RLE+ data header has invalid version";
    case (RLEPlusDecodeError::kDataIndexFailure):
      return "RLE+ incorrect structure";
    case (RLEPlusDecodeError::kUnpackOverflow):
      return "RLE+ invalid encoding";
    case (RLEPlusDecodeError::kMaxSizeExceed):
      return "RLE+ object size too large";
    default:
      return "RLE+ unknown error";
  }
}
