/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {
  using api::FullNodeApi;

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
    explicit ApiIpfsDatastore(std::shared_ptr<FullNodeApi> api);

    outcome::result<bool> contains(const CID &key) const override;

    /**
     * Set is not supported by API
     * @param key
     * @param value
     * @return Error not supported
     */
    outcome::result<void> set(const CID &key, Value value) override;

    outcome::result<Value> get(const CID &key) const override;

    std::shared_ptr<IpfsDatastore> shared() override;

   private:
    std::shared_ptr<FullNodeApi> api_;
  };

}  // namespace fc::storage::ipfs
