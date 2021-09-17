/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/throw_exception.hpp>
#include <libp2p/outcome/outcome.hpp>

#include "common/fmt_fwd.hpp"

/**
 * OUTCOME_EXCEPT raises exception in case of result has error.
 * Supports 2 forms:
 * OUTCOME_EXCEPT(expr);
 * OUTCOME_EXCEPT(var, expr); // var = expr
 */
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _OUTCOME_EXCEPT_1(expr) (expr).value()
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _OUTCOME_EXCEPT_2(val, expr) \
  auto(val) {                        \
    (expr).value()                   \
  }
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _OUTCOME_EXCEPT_OVERLOAD(_1, _2, NAME, ...) NAME
#define OUTCOME_EXCEPT(...)                                                   \
  _OUTCOME_EXCEPT_OVERLOAD(__VA_ARGS__, _OUTCOME_EXCEPT_2, _OUTCOME_EXCEPT_1) \
  (__VA_ARGS__)

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _OUTCOME_TRYA(var, val, ...) \
  OUTCOME_TRY(var, __VA_ARGS__);     \
  val = std::move(var);
#define OUTCOME_TRYA(val, ...) \
  _OUTCOME_TRYA(BOOST_OUTCOME_TRY_UNIQUE_NAME, val, __VA_ARGS__)

namespace fc::outcome {
  using libp2p::outcome::failure;
  using libp2p::outcome::result;
  using libp2p::outcome::success;

  /**
   * @brief throws outcome::result error as boost exception
   * @param t error code
   */
  [[noreturn]] inline void raise(const std::error_code &ec) {
    boost::throw_exception(std::system_error(ec));
  }

  /**
   * @brief throws outcome::result error as boost exception
   * @tparam T enum error type
   * @param t error value
   */
  template <typename T,
            typename = std::enable_if_t<std::is_error_code_enum_v<T>>>
  [[noreturn]] inline void raise(T t) {
    raise(make_error_code(t));
  }
}  // namespace fc::outcome

template <>
struct fmt::formatter<std::error_code, char, void>;

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _OUTCOME_CB1(u, r)  \
  auto && (u){r};           \
  if (!(u)) {               \
    return cb((u).error()); \
  }
#define OUTCOME_CB1(r) _OUTCOME_CB1(BOOST_OUTCOME_TRY_UNIQUE_NAME, r)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _OUTCOME_CB(u, l, r) \
  _OUTCOME_CB1(u, r)         \
  l = std::move((u).value());  // NOLINT(bugprone-macro-parentheses)
#define OUTCOME_CB(l, r) _OUTCOME_CB(BOOST_OUTCOME_TRY_UNIQUE_NAME, l, r)
