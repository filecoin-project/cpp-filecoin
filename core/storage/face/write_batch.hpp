/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_WRITE_BATCH_HPP
#define FILECOIN_WRITE_BATCH_HPP

#include "storage/face/writeable_map.hpp"

namespace fc::storage::face {

  /**
   * @brief An abstraction over a storage, which can be used for batch writes
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct WriteBatch : public WriteableMap<K, V> {
    /**
     * @brief Writes batch.
     * @return error code in case of error.
     */
    virtual outcome::result<void> commit() = 0;

    /**
     * @brief Clear batch.
     */
    virtual void clear() = 0;
  };

}  // namespace fc::storage::face

#endif  // FILECOIN_WRITE_BATCH_HPP
