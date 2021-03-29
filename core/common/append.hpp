/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

namespace fc {
  template <typename T>
  inline void append(std::vector<T> &l, const std::vector<T> &r) {
    l.insert(l.end(), r.begin(), r.end());
  }
}  // namespace fc
