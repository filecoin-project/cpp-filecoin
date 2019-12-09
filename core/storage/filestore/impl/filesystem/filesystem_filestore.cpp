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
using fc::storage::filestore::Path;

fc::outcome::result<bool> FileSystemFileStore::exists(const Path &path) const
    noexcept {
  return boost::filesystem::exists(path);
}

fc::outcome::result<std::shared_ptr<File>> FileSystemFileStore::open(
    const Path &path) noexcept {
  try {
    auto file = std::make_shared<FileSystemFile>(path);
    OUTCOME_TRY(file->open());
    return file;
  } catch (std::exception &) {
    return FileStoreError::UNKNOWN;
  }
}

fc::outcome::result<std::shared_ptr<File>> FileSystemFileStore::create(
    const Path &path) noexcept {
  try {
    // create file
    boost::filesystem::ofstream os(path);
    os.close();
    return open(path);
  } catch (std::exception &) {
    return FileStoreError::UNKNOWN;
  }
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

fc::outcome::result<std::vector<Path>> FileSystemFileStore::list(
    const Path &directory) noexcept {
  if (!boost::filesystem::exists(directory))
    return FileStoreError::DIRECTORY_NOT_FOUND;
  if (!boost::filesystem::is_directory(directory))
    return FileStoreError::NOT_DIRECTORY;
  std::vector<Path> res{};
  for (boost::filesystem::directory_iterator itr(directory);
       itr != boost::filesystem::directory_iterator();
       ++itr) {
    res.push_back(boost::filesystem::canonical(itr->path()).string());
  }

  return std::move(res);
}
