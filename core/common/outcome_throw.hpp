/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_OUTCOME_THROW_HPP
#define CPP_FILECOIN_CORE_COMMON_OUTCOME_THROW_HPP

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

namespace fc::outcome {
  /**
   * @brief throws outcome::result error as boost exception
   * @tparam T enum error type
   * @param t error value
   */
  template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
  void raise(T t) {
    std::error_code ec = make_error_code(t);
    boost::throw_exception(std::system_error(ec));
  }
}  // namespace fc::outcome

#endif  // CPP_FILECOIN_CORE_COMMON_OUTCOME_THROW_HPP
