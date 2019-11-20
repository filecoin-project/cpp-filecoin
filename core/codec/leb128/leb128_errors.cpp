/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/leb128/leb128_errors.hpp"

/**
 * @brief Outcome errors text definitions
 */
OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::leb128, LEB128DecodeError, e) {
  using fc::codec::leb128::LEB128DecodeError;
  switch (e) {
    case (LEB128DecodeError::INPUT_EMPTY):
      return "LEB128 decode: input value is empty";
    case (LEB128DecodeError::INPUT_TOO_BIG):
      return "LEB128 decode: decoded value overflow";
    default:
      return "LEB128: unknown error";
  }
}
