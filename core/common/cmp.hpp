/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define FC_OPERATOR_NOT_EQUAL_2(L, R)              \
  inline bool operator!=(const L &l, const R &r) { \
    return !(l == r);                              \
  }
#define FC_OPERATOR_NOT_EQUAL(T) FC_OPERATOR_NOT_EQUAL_2(T, T)

namespace fc {
  inline auto less() {
    return false;
  }

  template <typename T, typename... Ts>
  inline auto less(const T &l, const T &r, const Ts &...ts) {
    return l < r || (!(r < l) && less(ts...));
  }
}  // namespace fc
