/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "storage/face/map_cursor.hpp"

namespace fc::storage::face {

  /**
   * @brief An abstraction over read-only MAP.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct ReadableMap {
    virtual ~ReadableMap() = default;

    /**
     * @brief Get value by key
     * @param key K
     * @return V
     */
    virtual outcome::result<V> get(const K &key) const = 0;

    /**
     * @brief Returns true if given key-value binding exists in the storage.
     * @param key K
     * @return true if key has value, false otherwise.
     */
    virtual bool contains(const K &key) const = 0;
  };

}  // namespace fc::storage::face
