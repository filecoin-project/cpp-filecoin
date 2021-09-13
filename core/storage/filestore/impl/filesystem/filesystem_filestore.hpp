/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/filestore/filestore.hpp"

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

    /** @copydoc FileStore::createDirectories() */
    outcome::result<void> createDirectories(const Path &path) override;

    /** @copydoc FileStore::remove() */
    fc::outcome::result<void> remove(const Path &path) noexcept override;

    /** @copydoc FileStore::list() */
    outcome::result<std::vector<Path>> list(
        const Path &directory) noexcept override;
  };

}  // namespace fc::storage::filestore
