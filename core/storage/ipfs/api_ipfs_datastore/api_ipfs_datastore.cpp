/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore_error.hpp"

namespace fc::storage::ipfs {

  ApiIpfsDatastore::ApiIpfsDatastore(std::shared_ptr<FullNodeApi> api)
      : api_{api} {}

  outcome::result<bool> ApiIpfsDatastore::contains(const CID &key) const {
    return api_->ChainReadObj(key).has_value();
  }

  outcome::result<void> ApiIpfsDatastore::set(const CID &key,
                                              BytesCow &&value) {
    return ApiIpfsDatastoreError::kNotSupproted;
  }

  outcome::result<IpfsDatastore::Value> ApiIpfsDatastore::get(
      const CID &key) const {
    return api_->ChainReadObj(key);
  }
}  // namespace fc::storage::ipfs
