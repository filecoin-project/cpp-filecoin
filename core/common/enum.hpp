/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <string_view>
#include <type_traits>
#include <boost/optional.hpp>

namespace fc::common {
  template<typename Enumeration, size_t Number>
  using ConversionTable = std::array<std::pair<Enumeration, std::string_view>, Number>;

  template <typename T>
  auto& conversion_table() {
    return class_conversion_table(T{});
  }


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
   *   int v = to_int(Enumerator::eleven); // v == 11
   *   @nocode
   *
   * @tparam Enumeration - enum type
   * @param value - to convert
   * @return integer value of enum
   */
  template <typename Enumeration>
  auto to_int(Enumeration const value) ->
      typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
  }

  /**
   * @brief Convert enum class value as string
   * Usage:
   *   Given an enum class
   *   @code
   *   enum class Enumerator {
   *     one = 1,
   *     eleven = 2,
   *     hundred = 3
   *   }
   *   @nocode
   *   Can be converted as:
   *   @code
   *   boost::optional<std::string> result = to_string(Enumerator);
   *   @nocode
   *
   * @tparam Enumeration - enum type
   * @param value - to convert
   * @return string value of enum
   */
  template <typename Enumeration>
  boost::optional<std::string_view> to_string(
      Enumeration const value) {
    for (auto &[enumerator, str] : conversion_table<Enumeration>()) {
      if (enumerator == value) {
        return str;
      }
    }
    return boost::none;
  }

  /**
   * @brief Convert string to a enum value
   * Usage:
   *   Given an enum class
   *   @code
   *   enum class Enumerator {
   *     one = 1,
   *     eleven = 2,
   *     hundred = 3
   *   }
   *   @nocode
   *   Can be converted as:
   *   @code
   *   Enumeration result = to_string(value);
   *   @nocode
   *
   * @tparam Enumeration - enum type
   * @param value - to convert
   * @return string value of enum
   */
  template <typename Enumeration>
  boost::optional<Enumeration> from_string(
      const std::string_view value) {
    for (auto &[enumerator, str] : conversion_table<Enumeration>()) {
      if (str == value) {
        return enumerator;
      }
    }
    return boost::none;
  }

}  // namespace fc::common
