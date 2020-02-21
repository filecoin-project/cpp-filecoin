/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_IMPL_CHAIN_DATA_STORE_IMPL_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_IMPL_CHAIN_DATA_STORE_IMPL_HPP

#include "storage/chain/chain_data_store.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::blockchain {

  class ChainDataStoreImpl : public ChainDataStore {
   public:
    explicit ChainDataStoreImpl(std::shared_ptr<ipfs::IpfsDatastore> store);

    outcome::result<std::string> get(const DatastoreKey &key) const;

    outcome::result<void> set(const DatastoreKey &key, std::string_view value);

    outcome::result<bool> contains(const DatastoreKey &key) const;

    outcome::result<void> remove(const DatastoreKey &key);

   private:
    std::shared_ptr<ipfs::IpfsDatastore> store_;  ///< underlying storage
  };

}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_IMPL_CHAIN_DATA_STORE_IMPL_HPP
