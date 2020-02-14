/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_DATA_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_DATA_STORE_HPP

#include "storage/ipfs/datastore.hpp"
#include "storage/chain/datastore_key.hpp"

namespace fc::storage::blockchain {

  class ChainDataStore {
   public:
    explicit ChainDataStore(std::shared_ptr<ipfs::IpfsDatastore> store);

    /** get value by key */
    outcome::result<std::string> get(const DatastoreKey &key) const;
    /** set key -> value */
    outcome::result<void> set(const DatastoreKey &key, std::string_view value);
    /** checks whether store contains key */
    outcome::result<bool> contains(const DatastoreKey &key) const;

   private:
    std::shared_ptr<ipfs::IpfsDatastore> store_; ///< underlying storage
  };

}

#endif //CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_DATA_STORE_HPP
