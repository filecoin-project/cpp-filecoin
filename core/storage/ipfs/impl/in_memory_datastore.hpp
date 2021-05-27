/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {

  class InMemoryDatastore
      : public IpfsDatastore,
        public std::enable_shared_from_this<InMemoryDatastore> {
   public:
    ~InMemoryDatastore() override = default;

    /** @copydoc IpfsDatastore::contains() */
    outcome::result<bool> contains(const CID &key) const override;

    /** @copydoc IpfsDatastore::set() */
    outcome::result<void> set(const CID &key, Value value) override;

    /** @copydoc IpfsDatastore::get() */
    outcome::result<Value> get(const CID &key) const override;

    IpldPtr shared() override {
      return shared_from_this();
    }

   private:
    std::map<CID, Value> storage_;
  };

}  // namespace fc::storage::ipfs
