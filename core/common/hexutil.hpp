/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>
#include <vector>

#include <gsl/span>
#include "common/outcome.hpp"

namespace fc::common {

  /**
   * @brief error codes for exceptions that may occur during unhexing
   */
  enum class UnhexError { kNotEnoughInput = 1, kNonHexInput, kUnknown };

  /**
   * @brief Converts bytes to uppercase hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_upper(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Converts bytes to hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_lower(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Converts hex representation to bytes
   * @param array individual chars
   * @param len length of chars
   * @return result containing array of bytes if input string is hex encoded and
   * has even length
   *
   * @note reads both uppercase and lowercase hexstrings
   *
   * @see
   * https://www.boost.org/doc/libs/1_51_0/libs/algorithm/doc/html/the_boost_algorithm_library/Misc/hex.html
   */
  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex);

}  // namespace fc::common

OUTCOME_HPP_DECLARE_ERROR(fc::common, UnhexError);
