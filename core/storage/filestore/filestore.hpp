/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESTORE_FILESTORE_HPP
#define FILECOIN_CORE_STORAGE_FILESTORE_FILESTORE_HPP

#include <memory>
#include <string>
#include <vector>

#include "common/outcome.hpp"
#include "storage/filestore/file.hpp"
#include "storage/filestore/path.hpp"

namespace fc::storage::filestore {

  /**
   * @brief Interface of underlying system to store files.
   */
  class FileStore {
   public:
    virtual ~FileStore() = default;

    /**
     * @brief Does file exists
     * @return true if exists, false otherwise
     */
    virtual outcome::result<bool> exists(const Path &path) const noexcept = 0;

    /**
     * @brief Open file
     * @param path to a file
     * @return opened file
     */
    virtual outcome::result<std::shared_ptr<File>> open(
        const Path &path) noexcept = 0;

    /**
     * @brief Create file
     * @param path to a file
     * @return created file (not opened)
     */
    virtual outcome::result<std::shared_ptr<File>> create(
        const Path &path) noexcept = 0;

    /**
     * @brief Delete file
     * @param path to a file
     * @return filestore error on failure
     */
    virtual fc::outcome::result<void> remove(const Path &path) noexcept = 0;

    /**
     * @brief List all files in directory
     * @param directory path to a directory to list files
     * @return list of files
     */
    virtual outcome::result<std::vector<Path>> list(
        const Path &directory) noexcept = 0;
  };

}  // namespace fc::storage::filestore

#endif  // FILECOIN_CORE_STORAGE_FILESTORE_FILESTORE_HPP
