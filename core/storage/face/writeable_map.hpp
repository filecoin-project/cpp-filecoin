/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_WRITEABLE_MAP_HPP
#define CPP_FILECOIN_WRITEABLE_MAP_HPP

#include "common/outcome.hpp"

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
    virtual outcome::result<void> put(const K &key, const V &value) = 0;
    virtual outcome::result<void> put(const K &key, V&& value) = 0;

    /**
     * @brief Remove value by key
     * @param key K
     * @return error code if error happened
     */
    virtual outcome::result<void> remove(const K &key) = 0;
  };

}  // namespace fc::storage::face

#endif  // CPP_FILECOIN_WRITEABLE_MAP_HPP
