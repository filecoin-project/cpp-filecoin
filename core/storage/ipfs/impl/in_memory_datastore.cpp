/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/in_memory_datastore.hpp"

using fc::storage::ipfs::InMemoryDatastore;
using Value = fc::storage::ipfs::IpfsDatastore::Value;

fc::outcome::result<bool> InMemoryDatastore::contains(const CID &key) const {
  return storage_.find(key) != storage_.end();
}

fc::outcome::result<void> InMemoryDatastore::set(const CID &key, Value value) {
  storage_[key] = std::move(value);
  return fc::outcome::success();
}

fc::outcome::result<void> InMemoryDatastore::set(const CID &key,
                                                 SpanValue value) {
  storage_[key] = copy(value);
  return fc::outcome::success();
}

fc::outcome::result<Value> InMemoryDatastore::get(const CID &key) const {
  OUTCOME_TRY(found, contains(key));
  if (!found) {
    return IpfsDatastoreError::kNotFound;
  }
  return storage_.at(key);
}
