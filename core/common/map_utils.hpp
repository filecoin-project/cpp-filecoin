/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

namespace fc::common {

  /**
   * @returns map value if present, otherwise default_value
   */
  template <template <class, class, class...> class C,
            typename K,
            typename V,
            typename... Args>
  const V &getOrDefault(const C<K, V, Args...> &map,
                 const K &key,
                 const V &default_value) {
    const auto &found = map.find(key);
    if (found == map.end()) return default_value;
    return found->second;
  }
}  // namespace fc::common
