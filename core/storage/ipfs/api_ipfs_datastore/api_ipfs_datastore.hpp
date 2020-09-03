/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_IPFS_API_IPFS_DATASTORE_API_IPFS_DATASTORE_HPP
#define CPP_FILECOIN_STORAGE_IPFS_API_IPFS_DATASTORE_API_IPFS_DATASTORE_HPP

#include "api/api.hpp"
#include "storage/ipfs/ipfs_datastore.hpp"

namespace fc::storage::ipfs {
  using api::Api;

  /**
   * Read-only implementation of IPFS over node API
   */
  class ApiIpfsDatastore
      : public IpfsDatastore,
        public std::enable_shared_from_this<ApiIpfsDatastore> {
   public:
    /**
     * Construct ApiIpfsDatastore
     * @param api - node API
     */
    explicit ApiIpfsDatastore(std::shared_ptr<Api> api);

    outcome::result<bool> contains(const CID &key) const override;

    /**
     * Set is not supported by API
     * @param key
     * @param value
     * @return Error not supported
     */
    outcome::result<void> set(const CID &key, Value value) override;

    outcome::result<Value> get(const CID &key) const override;

    /**
     * Remove is not supported by API
     * @param key
     * @return Error not supported
     */
    outcome::result<void> remove(const CID &key) override;

    std::shared_ptr<IpfsDatastore> shared() override;

   private:
    std::shared_ptr<Api> api_;
  };

}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_STORAGE_IPFS_API_IPFS_DATASTORE_API_IPFS_DATASTORE_HPP
