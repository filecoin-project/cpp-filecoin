/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_ENUM_HPP
#define CPP_FILECOIN_CORE_COMMON_ENUM_HPP

namespace fc::common {

  /**
   * @brief Convert enum class value as integer
   * Usage:
   *   Given an enum class
   *   @code
   *   enum class Enumerator {
   *     one = 1,
   *     eleven = 11,
   *     hundred = 100
   *   }
   *   @nocode
   *   Can be converted as:
   *   @code
   *   int v = as_integer(Enumerator::eleven); // v == 11
   *   @nocode
   *
   * @tparam Enumeration - enum type
   * @param value - to convert
   * @return integer value of enum
   */
  template <typename Enumeration>
  auto as_integer(Enumeration const value) ->
      typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
  }

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_ENUM_HPP
