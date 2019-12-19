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
  auto res = storage_.emplace(key, value);
  if (!res.second) return IpfsDatastoreError::CANNOT_STORE;

  return fc::outcome::success();
}

fc::outcome::result<Value> InMemoryDatastore::get(const CID &key) const {
  OUTCOME_TRY(found, contains(key));
  if (!found) return IpfsDatastoreError::NOT_FOUND;
  return storage_.at(key);
}

fc::outcome::result<void> InMemoryDatastore::remove(const CID &key) {
  storage_.erase(key);
  return fc::outcome::success();
}
