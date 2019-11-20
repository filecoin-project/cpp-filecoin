/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_CODEC_LEB128_ERRORS_HPP
#define CORE_CODEC_LEB128_ERRORS_HPP

#include "common/outcome.hpp"

namespace fc::codec::leb128 {
  /**
   * @class Possible decode errors, compatible with std::error_code
   */
  enum class LEB128DecodeError : int {
    INPUT_EMPTY = 1, /**< Empty input is considered as error */
    INPUT_TOO_BIG    /**< Provided byte-vector will overflow decoded value */
  };
};  // namespace fc::codec::leb128

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::codec::leb128, LEB128DecodeError);

#endif  // CORE_CODEC_LEB128_ERRORS_HPP
