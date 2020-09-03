/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore_error.hpp"

namespace fc::storage::ipfs {

  ApiIpfsDatastore::ApiIpfsDatastore(std::shared_ptr<Api> api) : api_{api} {}

  outcome::result<bool> ApiIpfsDatastore::contains(const CID &key) const {
    auto res = api_->ChainReadObj(key);
    if (res.has_error()) {
      if (res.error() == IpfsDatastoreError::kNotFound) {
        return false;
      }
      return res.error();
    }
    return true;
  }

  outcome::result<void> ApiIpfsDatastore::set(const CID &key, Value value) {
    return ApiIpfsDatastoreError::kNotSupproted;
  }

  outcome::result<IpfsDatastore::Value> ApiIpfsDatastore::get(
      const CID &key) const {
    return api_->ChainReadObj(key);
  }

  outcome::result<void> ApiIpfsDatastore::remove(const CID &key) {
    return ApiIpfsDatastoreError::kNotSupproted;
  }

  std::shared_ptr<IpfsDatastore> ApiIpfsDatastore::shared() {
    return shared_from_this();
  }

}  // namespace fc::storage::ipfs
