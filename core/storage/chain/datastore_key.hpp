/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP

#include <string>

#include "common/outcome.hpp"

namespace fc::storage {
  /**
   * @struct Key datastore key
   */
  struct DatastoreKey {
    std::string value;
  };

  enum class DatastoreKeyError {
    INVALID_DATASTORE_KEY = 1, /// invalid data used for creating datastore key
  };

  /**
   * @brief creates key from string
   * @param value key data
   */
  DatastoreKey makeKeyFromString(std::string_view value);

  /**
   * @brief creates raw key from string without safety checking
   * @param value key data
   */
  outcome::result<DatastoreKey> makeRawKey(std::string_view value);

  inline bool operator<(const DatastoreKey &lhs, const DatastoreKey &rhs);

  bool operator==(const DatastoreKey &lhs, const DatastoreKey &rhs);

}  // namespace fc::storage::ipfs::key

OUTCOME_HPP_DECLARE_ERROR(fc::storage, DatastoreKeyError);

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP
