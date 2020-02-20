/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_BLOCKSERVICE_IMPL_HPP
#define FILECOIN_STORAGE_IPFS_BLOCKSERVICE_IMPL_HPP

#include <memory>

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {
  class IpfsBlockService : public IpfsDatastore {
   public:
    /**
     * @brief Construct IPFS storage
     * @param data_store - IPFS storage implementation
     */
    explicit IpfsBlockService(std::shared_ptr<IpfsDatastore> data_store);

    outcome::result<bool> contains(const CID &key) const override;

    outcome::result<void> set(const CID &key, Value value) override;

    outcome::result<Value> get(const CID &key) const override;

    outcome::result<void> remove(const CID &key) override;

   private:
    std::shared_ptr<IpfsDatastore> local_storage_; /**< Local data storage */
  };
}  // namespace fc::storage::ipfs

#endif
