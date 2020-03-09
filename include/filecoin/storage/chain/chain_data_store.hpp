/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_DATA_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_DATA_STORE_HPP

#include "filecoin/storage/chain/datastore_key.hpp"

namespace fc::storage::blockchain {

  /**
   * @class ChainDataStore is used by ChainStore as a map vec<CID> -> string
   */
  class ChainDataStore {
   public:
    virtual ~ChainDataStore() = default;
    /** @brief get value by key */
    virtual outcome::result<std::string> get(const DatastoreKey &key) const = 0;
    /** @brief set key -> value */
    virtual outcome::result<void> set(const DatastoreKey &key,
                                      std::string_view value) = 0;
    /** @brief checks whether store contains key */
    virtual outcome::result<bool> contains(const DatastoreKey &key) const = 0;
    /** @brief removes value by key */
    virtual outcome::result<void> remove(const DatastoreKey &key) = 0;
  };

}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_DATA_STORE_HPP
