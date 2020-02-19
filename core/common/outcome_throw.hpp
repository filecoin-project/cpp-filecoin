/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_OUTCOME_THROW_HPP
#define CPP_FILECOIN_CORE_COMMON_OUTCOME_THROW_HPP

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define _OUTCOME_EXCEPT(var, val, expr)        \
  auto &&var = expr;                           \
  if (!var) ::fc::outcome::raise(var.error()); \
  auto &&val = var.value();
#define OUTCOME_EXCEPT(val, expr) _OUTCOME_EXCEPT(UNIQUE_NAME(_r), val, expr)

namespace fc::outcome {
  /**
   * @brief throws outcome::result error as boost exception
   * @param t error code
   */
  inline void raise(const std::error_code &ec) {
    boost::throw_exception(std::system_error(ec));
  }

  /**
   * @brief throws outcome::result error as boost exception
   * @tparam T enum error type
   * @param t error value
   */
  template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
  void raise(T t) {
    raise(make_error_code(t));
  }
}  // namespace fc::outcome

#endif  // CPP_FILECOIN_CORE_COMMON_OUTCOME_THROW_HPP
