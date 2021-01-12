/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc {
  inline auto less() {
    return false;
  }

  template <typename T, typename... Ts>
  inline auto less(const T &l, const T &r, const Ts &... ts) {
    return l < r || (!(r < l) && less(ts...));
  }
}  // namespace fc
