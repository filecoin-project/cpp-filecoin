/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESYSTEM_KEYSTORE_HPP
#define FILECOIN_CORE_STORAGE_FILESYSTEM_KEYSTORE_HPP

#include "storage/filestore/filestore.hpp"
#include "storage/filestore/path.hpp"
#include "storage/keystore/keystore.hpp"

namespace fc::storage::keystore {

  using filestore::FileStore;
  using filestore::Path;

  /**
   * @brief FileSystem KeyStore implementation
   */
  class FileSystemKeyStore : public KeyStore {
    /** @brief Extention of private key file */
    const std::string kPrivateKeyExtension = ".pri";

   public:
    FileSystemKeyStore(Path path,
                       std::shared_ptr<BlsProvider> blsProvider,
                       std::shared_ptr<Secp256k1Provider> secp256K1Provider);

    ~FileSystemKeyStore() override = default;

    /** @copydoc KeyStore::has() */
    outcome::result<bool> has(const Address &address) const noexcept override;

    /** @copydoc KeyStore::put() */
    outcome::result<void> put(
        Address address, typename KeyStore::TPrivateKey key) noexcept override;

    /** @copydoc KeyStore::remove() */
    outcome::result<void> remove(const Address &address) noexcept override;

    /** @copydoc KeyStore::list() */
    outcome::result<std::vector<Address>> list() const noexcept override;

   protected:
    /** @copydoc KeyStore::get() */
    outcome::result<typename KeyStore::TPrivateKey> get(
        const Address &address) const noexcept override;

   private:
    /**
     * @brief Get path to private key file from address
     * @param address
     * @return Path to private key
     */
    outcome::result<Path> addressToPath(const Address &address) const noexcept;

    /** Path to directory with keys */
    Path keystore_path_;

    std::shared_ptr<FileStore> filestore_;
  };

}  // namespace fc::storage::keystore

#endif  // FILECOIN_CORE_STORAGE_FILESYSTEM_KEYSTORE_HPP
