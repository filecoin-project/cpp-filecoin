/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/face/iterable_map.hpp"
#include "storage/face/readable_map.hpp"
#include "storage/face/writeable_map.hpp"

namespace fc::storage::face {

  /**
   * @brief An abstraction over readable, writeable, iterable key-value map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct GenericMap : public IterableMap<K, V>,
                      public ReadableMap<K, V>,
                      public WriteableMap<K, V> {};
}  // namespace fc::storage::face
