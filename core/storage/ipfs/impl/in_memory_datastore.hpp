/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {

  class InMemoryDatastore : public Ipld {
   public:
    ~InMemoryDatastore() override = default;

    /** @copydoc IpfsDatastore::contains() */
    outcome::result<bool> contains(const CID &key) const override;

    /** @copydoc IpfsDatastore::set() */
    outcome::result<void> set(const CID &key, Value value) override;
    outcome::result<void> set(const CID &key, SpanValue value) override;

    /** @copydoc IpfsDatastore::get() */
    outcome::result<Value> get(const CID &key) const override;

   private:
    std::map<CID, Value> storage_;
  };

}  // namespace fc::storage::ipfs
