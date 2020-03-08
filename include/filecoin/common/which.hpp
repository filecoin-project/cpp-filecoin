/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_WHICH_HPP
#define CPP_FILECOIN_CORE_COMMON_WHICH_HPP

#include <boost/variant.hpp>

namespace fc::common {
  /**
   * which<T>(boost::variant<...>) checks if type T is currently held by
   * boost::variant. Wraps boost::variant<...>::which(). Allows to query by
   * type, not by index.
   */

  template <typename T, typename T2>
  bool which(const boost::variant<T, T2> &v) {
    return v.which() == 0;
  }

  template <typename T, typename T1>
  bool which(const boost::variant<T1, T> &v) {
    return v.which() == 1;
  }

  template <typename T, typename T2, typename T3>
  bool which(const boost::variant<T, T2, T3> &v) {
    return v.which() == 0;
  }

  template <typename T, typename T1, typename T3>
  bool which(const boost::variant<T1, T, T3> &v) {
    return v.which() == 1;
  }

  template <typename T, typename T1, typename T2>
  bool which(const boost::variant<T1, T2, T> &v) {
    return v.which() == 2;
  }
}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_WHICH_HPP
