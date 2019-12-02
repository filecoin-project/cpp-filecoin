/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/filestore/impl/filesystem/filesystem_filestore.hpp"
#include "boost/filesystem.hpp"
#include "storage/filestore/filestore_error.hpp"
#include "storage/filestore/impl/filesystem/filesystem_file.hpp"

using fc::storage::filestore::File;
using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::FileSystemFileStore;

fc::outcome::result<std::shared_ptr<File>> FileSystemFileStore::open(
    const Path &path) noexcept {
  auto file = std::make_shared<FileSystemFile>(path);
  OUTCOME_TRY(file->open());

  return file;
}

fc::outcome::result<std::shared_ptr<File>> FileSystemFileStore::create(
    const Path &path) noexcept {
  // create file
  boost::filesystem::ofstream os(path);
  os.close();

  return open(path);
}

fc::outcome::result<void> FileSystemFileStore::remove(
    const Path &path) noexcept {
  if (!boost::filesystem::exists(path)) return FileStoreError::FILE_NOT_FOUND;

  boost::system::error_code error_code;
  boost::filesystem::remove(path, error_code);

  if (error_code != boost::system::errc::success)
    return FileStoreError::UNKNOWN;

  return fc::outcome::success();
}
