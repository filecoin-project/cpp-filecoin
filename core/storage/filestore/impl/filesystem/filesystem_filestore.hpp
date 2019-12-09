/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESTORE_FILESYSTEM_FILESTORE_HPP
#define FILECOIN_CORE_STORAGE_FILESTORE_FILESYSTEM_FILESTORE_HPP

#import "storage/filestore/filestore.hpp"

namespace fc::storage::filestore {

  /**
   * @brief Boost filesystem implementation of FileStore
   */
  class FileSystemFileStore : public virtual FileStore {
   public:
    ~FileSystemFileStore() override = default;

    /** @copydoc FileStore::exists() */
    outcome::result<bool> exists(const Path &path) const noexcept override;

    /** @copydoc FileStore::open() */
    outcome::result<std::shared_ptr<File>> open(
        const Path &path) noexcept override;

    /** @copydoc FileStore::create() */
    fc::outcome::result<std::shared_ptr<File>> create(
        const Path &path) noexcept override;

    /** @copydoc FileStore::remove() */
    fc::outcome::result<void> remove(const Path &path) noexcept override;

    /** @copydoc FileStore::list() */
    outcome::result<std::vector<Path>> list(
        const Path &directory) noexcept override;
  };

}  // namespace fc::storage::filestore

#endif  // FILECOIN_CORE_STORAGE_FILESTORE_FILESYSTEM_FILESTORE_HPP
