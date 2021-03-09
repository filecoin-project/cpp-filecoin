/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP
#define CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <sstream>

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

/**
 * OUTCOME_EXCEPT raises exception in case of result has error.
 * Supports 2 forms:
 * OUTCOME_EXCEPT(expr);
 * OUTCOME_EXCEPT(var, expr); // var = expr
 */
#define _OUTCOME_EXCEPT_1(var, expr) \
  auto &&var = expr;                 \
  if (!var) ::fc::outcome::raise(var.error());
#define _OUTCOME_EXCEPT_2(var, val, expr) \
  _OUTCOME_EXCEPT_1(var, expr)            \
  auto &&val = var.value();
#define _OUTCOME_EXCEPT_OVERLOAD(_1, _2, NAME, ...) NAME
#define OUTCOME_EXCEPT(...)                                                   \
  _OUTCOME_EXCEPT_OVERLOAD(__VA_ARGS__, _OUTCOME_EXCEPT_2, _OUTCOME_EXCEPT_1) \
  (UNIQUE_NAME(_r), __VA_ARGS__)

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

  /**
   * Returns human readable error code string
   * @param ec - error code
   * @return string representation of error
   */
  inline std::string errorToPrettyString(const std::error_code &ec) {
    std::stringstream ss;
    ss << ec.category().name() << " error " << std::to_string(ec.value())
       << ": \"" << ec.message() + "\"";
    return ss.str();
  }

}  // namespace fc::outcome

namespace fmt {
  inline namespace v6 {
    template <typename T, typename Char, typename Enable>
    struct formatter;
  }  // namespace v6

  template <>
  struct formatter<std::error_code, char, void>;
}  // namespace fmt

#define _OUTCOME_ALTERNATIVE(res, var, expression, alternative) \
  auto &&res = (expression);                                    \
  auto &&var = (res) ? (res.value()) : (alternative);

#define OUTCOME_ALTERNATIVE(var, expression, alternative) \
  _OUTCOME_ALTERNATIVE(UNIQUE_NAME(_r), var, expression, alternative)

#define _OUTCOME_CB1(u, r) \
  auto &&u{r};             \
  if (!u) {                \
    return cb(u.error());  \
  }
#define OUTCOME_CB1(r) _OUTCOME_CB1(BOOST_OUTCOME_TRY_UNIQUE_NAME, r)
#define _OUTCOME_CB(u, l, r) \
  _OUTCOME_CB1(u, r)         \
  l = std::move(u.value());
#define OUTCOME_CB(l, r) _OUTCOME_CB(BOOST_OUTCOME_TRY_UNIQUE_NAME, l, r)

#endif  // CPP_FILECOIN_CORE_COMMON_OUTCOME_HPP
