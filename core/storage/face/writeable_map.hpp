/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "storage/face/cow.hpp"

namespace fc::storage::face {

  /**
   * @brief An abstraction over modifiable map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct WriteableMap {
    virtual ~WriteableMap() = default;

    /**
     * @brief Store value by key
     * @param key key
     * @param value value
     * @return result containing void if put successful, error otherwise
     */
    virtual outcome::result<void> put(const K &key, cow_t<V> &&value) = 0;

    /**
     * @brief Remove value by key
     * @param key K
     * @return error code if error happened
     */
    virtual outcome::result<void> remove(const K &key) = 0;
  };

}  // namespace fc::storage::face
