/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP

#include <string>

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/common/outcome.hpp"

namespace fc::storage {
  /**
   * @struct Key datastore key
   */
  struct DatastoreKey {
    std::string value;

    /**
     * @brief creates key from string
     * @param value key data
     */
    static DatastoreKey makeFromString(std::string_view value) noexcept;
  };

  enum class DatastoreKeyError {
    INVALID_DATASTORE_KEY = 1,  /// invalid data used for creating datastore key
  };

  /** @brief cbor-encodes DatastoreKey instance */
  CBOR_ENCODE(DatastoreKey, key) {
    return s << (s.list() << key.value);
  }

  /** @brief cbor-decodes DatastoreKey instance */
  CBOR_DECODE(DatastoreKey, key) {
    s.list() >> key.value;
    return s;
  }

  /** @brief orders 2 DatastoreKey instances */
  bool operator<(const DatastoreKey &lhs, const DatastoreKey &rhs);

  /** @brief compares 2 DatastoreKey intances for equality */
  bool operator==(const DatastoreKey &lhs, const DatastoreKey &rhs);

}  // namespace fc::storage

OUTCOME_HPP_DECLARE_ERROR(fc::storage, DatastoreKeyError);

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP
