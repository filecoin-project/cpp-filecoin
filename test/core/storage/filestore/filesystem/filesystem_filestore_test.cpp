/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/filestore/impl/filesystem/filesystem_filestore.hpp"

#include <gtest/gtest.h>
#include "storage/filestore/filestore_error.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::filestore::FileStore;
using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFileStore;
using fc::storage::filestore::Path;

class FileSystemFileStoreTest : public test::BaseFS_Test {
 public:
  FileSystemFileStoreTest()
      : test::BaseFS_Test("/tmp/fc_filesystem_filestore_test") {}

  std::shared_ptr<FileStore> fs;

  /**
   * Create a test directory.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
    fs = std::make_shared<FileSystemFileStore>();
  }
};

/**
 * @given path to a directory
 * @when open file by path is called
 * @then error CANNOT_OPEN returned
 */
TEST_F(FileSystemFileStoreTest, TryToOpenDirectory) {
  Path path(fs::canonical(base_path ).string());
  auto file_res = fs->open(path);

  ASSERT_FALSE(file_res);
  ASSERT_EQ(FileStoreError::CANNOT_OPEN, file_res.error());
}

/**
 * @given path to file that doesn't exist
 * @when open file by path is called
 * @then error FILE_NOT_FOUND returned
 */
TEST_F(FileSystemFileStoreTest, FileNotFound) {
  auto filename = (base_path /= "not_exists.txt").string();
  Path path(filename);
  auto file_res = fs->open(path);

  ASSERT_FALSE(file_res);
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, file_res.error());
}

/**
 * @given path to file that exists
 * @when open file by path is called
 * @then open file is returned
 */
TEST_F(FileSystemFileStoreTest, OpenFile) {
  auto filename = fs::canonical(createFile("new_file.txt")).string();
  auto file_res = fs->open(filename);

  ASSERT_TRUE(file_res);
  ASSERT_TRUE(file_res.value()->is_open());
  ASSERT_EQ(filename, file_res.value()->path());
}

/**
 * @given path to file that doesn't exist
 * @when create file by path is called
 * @then file created and returned
 */
TEST_F(FileSystemFileStoreTest, CreateFile) {
  auto filename = fs::canonical(createFile("new_file.txt")).string();
  auto file_res = fs->create(filename);

  ASSERT_TRUE(file_res);
  ASSERT_TRUE(file_res.value()->is_open());
  ASSERT_EQ(filename, file_res.value()->path());
}

/**
 * @given path to file that exists
 * @when delete file by path is called
 * @then file deleted
 */
TEST_F(FileSystemFileStoreTest, RemoveFile) {
  auto filename = fs::canonical(createFile("to_delete.txt")).string();
  createFile(filename);
  ASSERT_TRUE(boost::filesystem::exists(filename));

  auto file_res = fs->remove(filename);

  ASSERT_TRUE(file_res);
  ASSERT_FALSE(boost::filesystem::exists(filename));
}
