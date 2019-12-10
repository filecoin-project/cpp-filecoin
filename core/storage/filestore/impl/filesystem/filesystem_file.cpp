/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/filestore/impl/filesystem/filesystem_file.hpp"

#include "boost/filesystem.hpp"
#include "storage/filestore/filestore_error.hpp"

using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::Path;

FileSystemFile::FileSystemFile(Path path) : path_(std::move(path)) {}

Path FileSystemFile::path() const noexcept {
  return path_;
}

fc::outcome::result<size_t> FileSystemFile::size() const noexcept {
  OUTCOME_TRY(file_exists, exists());
  if (!file_exists) return FileStoreError::FILE_NOT_FOUND;

  return boost::filesystem::file_size(path_);
}

fc::outcome::result<void> FileSystemFile::open() noexcept {
  OUTCOME_TRY(file_exists, exists());
  if (!file_exists) return FileStoreError::FILE_NOT_FOUND;

  fstream_.open(path_);

  if (fstream_.fail()) return FileStoreError::CANNOT_OPEN;
  return fc::outcome::success();
}

fc::outcome::result<void> FileSystemFile::close() noexcept {
  OUTCOME_TRY(file_exists, exists());
  try {
    if (!file_exists) return FileStoreError::FILE_NOT_FOUND;
    if (!fstream_.is_open()) return FileStoreError::FILE_CLOSED;

    fstream_.close();

    if (fstream_.fail()) return FileStoreError::UNKNOWN;
    return fc::outcome::success();
  } catch (std::exception &) {
    return FileStoreError::UNKNOWN;
  }
}

fc::outcome::result<size_t> FileSystemFile::read(
    size_t offset, const gsl::span<uint8_t> &buffer) noexcept {
  OUTCOME_TRY(file_exists, exists());
  if (!file_exists) return FileStoreError::FILE_NOT_FOUND;
  if (!fstream_.is_open()) return FileStoreError::FILE_CLOSED;
  if (fstream_.fail()) return FileStoreError::UNKNOWN;

  fstream_.seekg(offset, std::ios_base::beg);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  fstream_.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
  auto res = fstream_.gcount();

  return res;
}

fc::outcome::result<size_t> FileSystemFile::write(
    size_t offset, const gsl::span<const uint8_t> &buffer) noexcept {
  OUTCOME_TRY(file_exists, exists());
  if (!file_exists) return FileStoreError::FILE_NOT_FOUND;
  if (!fstream_.is_open()) return FileStoreError::FILE_CLOSED;
  if (fstream_.fail()) return FileStoreError::UNKNOWN;

  fstream_.seekp(offset, std::ios_base::beg);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  fstream_.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
  fstream_.flush();
  auto pos = fstream_.tellp();
  if (pos == -1) return FileStoreError::UNKNOWN;

  return static_cast<size_t>(pos) - offset;
}

bool FileSystemFile::is_open() const noexcept {
  return fstream_.is_open();
}

fc::outcome::result<bool> FileSystemFile::exists() const noexcept {
  boost::system::error_code error_code;
  auto res = boost::filesystem::exists(path_, error_code);
  if (error_code == boost::system::errc::no_such_file_or_directory)
    return FileStoreError::FILE_NOT_FOUND;
  if (error_code != boost::system::errc::success)
    return FileStoreError::UNKNOWN;

  return res;
}
