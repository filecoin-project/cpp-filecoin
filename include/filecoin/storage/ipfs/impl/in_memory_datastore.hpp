/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_IPFS_IMPL_IN_MEMORY_DATASTORE_HPP
#define CPP_FILECOIN_IPFS_IMPL_IN_MEMORY_DATASTORE_HPP

#include <map>

#include "filecoin/storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {

  class InMemoryDatastore : public IpfsDatastore {
   public:
    ~InMemoryDatastore() override = default;

    /** @copydoc IpfsDatastore::contains() */
    outcome::result<bool> contains(const CID &key) const override;

    /** @copydoc IpfsDatastore::set() */
    outcome::result<void> set(const CID &key, Value value) override;

    /** @copydoc IpfsDatastore::get() */
    outcome::result<Value> get(const CID &key) const override;

    /** @copydoc IpfsDatastore::remove() */
    outcome::result<void> remove(const CID &key) override;

   private:
    std::map<CID, Value> storage_;
  };

}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_IPFS_IMPL_IN_MEMORY_DATASTORE_HPP
