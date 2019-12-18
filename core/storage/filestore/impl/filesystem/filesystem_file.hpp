/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESTORE_FILESYSTEM_FILE_HPP
#define FILECOIN_CORE_STORAGE_FILESTORE_FILESYSTEM_FILE_HPP

#include <string>

#include "boost/filesystem.hpp"
#include "storage/filestore/file.hpp"

namespace fc::storage::filestore {

  /**
   * Boost filesystem implementation of File
   */
  class FileSystemFile : public virtual File {
   public:
    explicit FileSystemFile(Path path);

    ~FileSystemFile() override = default;

    /** \copydoc File::path() */
    Path path() const noexcept override;

    /** \copydoc File::size() */
    outcome::result<size_t> size() const noexcept override;

    /** \copydoc File::open() */
    outcome::result<void> open() noexcept override;

    /** \copydoc File::close() */
    outcome::result<void> close() noexcept override;

    /** \copydoc File::read() */
    fc::outcome::result<size_t> read(
        size_t offset, gsl::span<uint8_t> buffer) noexcept override;

    /** \copydoc File::write() */
    fc::outcome::result<size_t> write(
        size_t offset, gsl::span<const uint8_t> buffer) noexcept override;

    /** \copydoc File::is_open() */
    bool is_open() const noexcept override;

    /** \copydoc File::exists() */
    fc::outcome::result<bool> exists() const noexcept override;

   private:
    Path path_;
    boost::filesystem::fstream fstream_;
  };

}  // namespace fc::storage::filestore

#endif  // FILECOIN_CORE_STORAGE_FILESTORE_FILESYSTEM_FILE_HPP
