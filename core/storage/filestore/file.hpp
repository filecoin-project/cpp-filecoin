/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESTORE_FILE_H
#define FILECOIN_CORE_STORAGE_FILESTORE_FILE_H

#include <gsl/span>

#include "common/outcome.hpp"
#include "storage/filestore/path.hpp"

namespace fc::storage::filestore {

  /**
   * @brief Interface of a filesystem file.
   */
  class File {
   public:
    virtual ~File() = default;

    /**
     * @brief get path to a file
     * @return path to a file
     */
    virtual Path path() const noexcept = 0;

    /**
     * @brief get size of the file
     * @return size of the file
     */
    virtual outcome::result<size_t> size() const noexcept = 0;

    /**
     * @brief open file
     */
    virtual outcome::result<void> open() noexcept = 0;

    /**
     * @brief close file
     */
    virtual outcome::result<void> close() noexcept = 0;

    /**
     * @brief read bytes from the file
     * @param offset start position to read from
     * @param buffer pointer to the first element to be read
     * @return number of bytes read
     */
    virtual fc::outcome::result<size_t> read(
        size_t offset, gsl::span<uint8_t> buffer) noexcept = 0;

    /**
     * @brief write to the file
     * @param offset start position to write to
     * @param buffer pointer to the first element of array to be written
     * @return number of bytes written
     */
    virtual fc::outcome::result<size_t> write(
        size_t offset, gsl::span<const uint8_t> buffer) noexcept = 0;

    /**
     * @brief Whether the file is open
     * @return true if file is open, false otherwise
     */
    virtual bool is_open() const noexcept = 0;

    /**
     * @brief Whether the file exists
     * @return true if file exists, false otherwise
     */
    virtual fc::outcome::result<bool> exists() const noexcept = 0;
  };

}  // namespace fc::storage::filestore

#endif  // FILECOIN_CORE_STORAGE_FILESTORE_FILE_H
