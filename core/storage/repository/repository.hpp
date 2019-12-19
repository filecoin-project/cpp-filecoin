/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_REPOSITORY_HPP
#define FILECOIN_CORE_STORAGE_REPOSITORY_HPP

#include "common/outcome.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/keystore/keystore.hpp"

namespace fc::storage::repository {

  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::KeyStore;

  // TODO(a.chernyshov) add config
  /**
   * @ brief Class represents all persistent data on node
   */
  class Repository {
   public:
    Repository(std::shared_ptr<IpfsDatastore> ipldStore,
               std::shared_ptr<KeyStore> keystore);

    virtual ~Repository() = default;

    /**
     * @brief Persistent data storage for small structured objects.
     * @return ipld storage
     */
    std::shared_ptr<IpfsDatastore> getIpldStore() const noexcept;

    /**
     * @brief Cryptoghraphy keys that are secret for filecoin node.
     * @return Keystore
     */
    std::shared_ptr<KeyStore> getKeyStore() const noexcept;

    /**
     * @brief User-editable configuration values.
     */

    /**
     * @brief A repo version is a single incrementing integer. All versions are
     * considered non-compatible.
     * @return version number
     */
    virtual fc::outcome::result<int> getVersion() const = 0;

   private:
    std::shared_ptr<IpfsDatastore> ipld_store_;
    std::shared_ptr<KeyStore> keystore_;
  };

}  // namespace fc::storage::repository

#endif  // FILECOIN_CORE_STORAGE_REPOSITORY_HPP
