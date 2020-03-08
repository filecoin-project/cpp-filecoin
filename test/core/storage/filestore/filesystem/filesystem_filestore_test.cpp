/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/filestore/impl/filesystem/filesystem_filestore.hpp"

#include <gtest/gtest.h>
#include "filecoin/storage/filestore/filestore_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::filestore::FileStore;
using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFileStore;
using fc::storage::filestore::Path;

class FileSystemFileStoreTest : public test::BaseFS_Test {
 public:
  FileSystemFileStoreTest()
      : test::BaseFS_Test("fc_filesystem_filestore_test") {}

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
 * @given path to file that doesn't exist
 * @when exists() called
 * @then false returned
 */
TEST_F(FileSystemFileStoreTest, ExistsNotExist) {
  Path path((base_path /= "not_exists.txt").string());
  EXPECT_OUTCOME_TRUE(res, fs->exists(path));
  EXPECT_FALSE(res);
}

/**
 * @given path to file that doesn't exist
 * @when exists() called
 * @then false returned
 */
TEST_F(FileSystemFileStoreTest, ExistsExist) {
  auto filename = fs::canonical(createFile("new_file.txt")).string();
  Path path(filename);
  EXPECT_OUTCOME_TRUE(res, fs->exists(path));
  EXPECT_TRUE(res);
}

/**
 * @given path to a directory
 * @when open file by path is called
 * @then error CANNOT_OPEN returned
 */
TEST_F(FileSystemFileStoreTest, TryToOpenDirectory) {
  Path path(fs::canonical(base_path).string());
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
  Path path((base_path /= "not_exists.txt").string());
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
  EXPECT_OUTCOME_TRUE(file, fs->open(filename));
  ASSERT_TRUE(file->is_open());
  ASSERT_EQ(filename, file->path());
}

/**
 * @given path to file that doesn't exist
 * @when create file by path is called
 * @then file created and returned
 */
TEST_F(FileSystemFileStoreTest, CreateFile) {
  auto filename = getPathString() + "/new_file.txt";
  EXPECT_OUTCOME_TRUE(file, fs->create(filename));
  ASSERT_TRUE(file->is_open());
  ASSERT_EQ(filename, file->path());
}

/**
 * @given path to file that exists
 * @when delete file by path is called
 * @then file deleted
 */
TEST_F(FileSystemFileStoreTest, RemoveFile) {
  auto filename = fs::canonical(createFile("to_delete.txt")).string();
  ASSERT_TRUE(boost::filesystem::exists(filename));

  EXPECT_OUTCOME_TRUE_1( fs->remove(filename));
  ASSERT_FALSE(boost::filesystem::exists(filename));
}

/**
 * @given path to dir that does not exist
 * @when list() is called
 * @then DIRECTORY_NOT_FOUND returned
 */
TEST_F(FileSystemFileStoreTest, ListDirectoryNotFound) {
  Path path((base_path /= "not_exists").string());
  auto res = fs->list(path);
  ASSERT_FALSE(res);
  ASSERT_EQ(FileStoreError::DIRECTORY_NOT_FOUND, res.error());
}

/**
 * @given path to file that exists
 * @when list() is called
 * @then NOT_DIRECTORY returned
 */
TEST_F(FileSystemFileStoreTest, ListFile) {
  auto filename = fs::canonical(createFile("file.txt")).string();
  auto res = fs->list(filename);
  ASSERT_FALSE(res);
  ASSERT_EQ(FileStoreError::NOT_DIRECTORY, res.error());
}

/**
 * @given path to empty dir
 * @when list() is called
 * @then empty list returned
 */
TEST_F(FileSystemFileStoreTest, EmptyList) {
  EXPECT_OUTCOME_TRUE(list, fs->list(base_path.string()));
  ASSERT_EQ(0, list.size());
}

/**
 * @given path to dir with files
 * @when list() is called
 * @then list of files returned
 */
TEST_F(FileSystemFileStoreTest, ListFiles) {
  std::set<Path> filenames;
  for (int i = 0; i < 3; i++) {
    auto filename = fs::canonical(createFile("file" + std::to_string(i) + ".txt")).string();
    filenames.insert(filename);
    ASSERT_TRUE(boost::filesystem::exists(filename));
  }

  EXPECT_OUTCOME_TRUE(list, fs->list(base_path.string()));
  std::set<Path> res(list.begin(), list.end());
  ASSERT_EQ(filenames, res);
}
