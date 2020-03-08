/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GENERIC_MAP_HPP
#define CPP_FILECOIN_GENERIC_MAP_HPP

#include "filecoin/storage/face/iterable_map.hpp"
#include "filecoin/storage/face/readable_map.hpp"
#include "filecoin/storage/face/writeable_map.hpp"

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

#endif  // CPP_FILECOIN_GENERIC_MAP_HPP
