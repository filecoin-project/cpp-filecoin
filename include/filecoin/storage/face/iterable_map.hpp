/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ITERABLE_MAP_HPP
#define CPP_FILECOIN_ITERABLE_MAP_HPP

#include <memory>

#include "filecoin/storage/face/map_cursor.hpp"

namespace fc::storage::face {

  /**
   * @brief An abstraction of a key-value map, that is iterable.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct IterableMap {
    virtual ~IterableMap() = default;

    /**
     * @brief Returns new key-value iterator.
     * @return kv iterator
     */
    virtual std::unique_ptr<MapCursor<K, V>> cursor() = 0;
  };

}  // namespace fc::storage::face

#endif  // CPP_FILECOIN_ITERABLE_MAP_HPP
