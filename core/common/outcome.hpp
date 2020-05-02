/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP
#define CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <libp2p/outcome/outcome.hpp>

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define _OUTCOME_EXCEPT(var, val, expr)        \
  auto &&var = expr;                           \
  if (!var) ::fc::outcome::raise(var.error()); \
  auto &&val = var.value();
#define OUTCOME_EXCEPT(val, expr) _OUTCOME_EXCEPT(UNIQUE_NAME(_r), val, expr)

#define _OUTCOME_TRYA(var, val, expr) \
  auto &&var = expr;                  \
  if (!var) return var.error();       \
  val = std::move(var.value());
#define OUTCOME_TRYA(val, expr) _OUTCOME_TRYA(UNIQUE_NAME(_r), val, expr)

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
  template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
  [[noreturn]] inline void raise(T t) {
    raise(make_error_code(t));
  }
}  // namespace fc::outcome

#define _OUTCOME_ALTERNATIVE(res, var, expression, alternative) \
  auto &&res = (expression);                                    \
  auto &&var = (res) ? (res.value()) : (alternative);

#define OUTCOME_ALTERNATIVE(var, expression, alternative) \
  _OUTCOME_ALTERNATIVE(UNIQUE_NAME(_r), var, expression, alternative)

#endif  // CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP
