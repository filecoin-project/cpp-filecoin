/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/filestore/impl/filesystem/filesystem_file.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "storage/filestore/filestore_error.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::filestore::File;
using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::Path;

class FileSystemFileTest : public test::BaseFS_Test {
 public:
  FileSystemFileTest() : test::BaseFS_Test("/tmp/fc_filesystem_file_test/") {}

  /** Path to empty file */
  Path empty_file_path;

  /** Empty file */
  std::shared_ptr<File> empty_file;

  /**
   * Create a test directory with an empty file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
    empty_file_path = fs::canonical(createFile("empty.txt")).string();
    empty_file = std::make_shared<FileSystemFile>(empty_file_path);
  }
};

/**
 * @given path to file that doesn't exist
 * @when open file is called
 * @then error FILE_NOT_FOUND returned for functions calls
 */
TEST_F(FileSystemFileTest, FileNotFound) {
  Path path("not/found/file.txt");
  auto file = std::make_shared<FileSystemFile>(path);
  auto res = file->open();

  ASSERT_FALSE(res);
  ASSERT_FALSE(file->is_open());
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, res.error());

  auto res_size = file->size();
  ASSERT_FALSE(res_size);

  char buff[] = "abc";

  auto write_res = file->write(0, 3, buff);
  ASSERT_FALSE(write_res);
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, write_res.error());

  auto read_res = file->read(0, 3, buff);
  ASSERT_FALSE(read_res);
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, read_res.error());

  auto close_res = file->close();
  ASSERT_FALSE(close_res);
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, close_res.error());
}

/**
 * @given file is opened
 * @when open file is called the second time
 * @then error CANNOT_OPEN returned
 */
TEST_F(FileSystemFileTest, DoubleOpen) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);
  ASSERT_TRUE(empty_file->is_open());
  ASSERT_EQ(empty_file_path, empty_file->path());

  // open again
  open_res = empty_file->open();

  ASSERT_FALSE(open_res);
  ASSERT_TRUE(empty_file->is_open());
  ASSERT_EQ(FileStoreError::CANNOT_OPEN, open_res.error());
}

/**
 * @given file is opened
 * @when close file is called
 * @then error FILE_CLOSED returned on method calls
 */
TEST_F(FileSystemFileTest, CloseFile) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);

  auto close_res = empty_file->close();
  ASSERT_TRUE(close_res);
  ASSERT_FALSE(empty_file->is_open());

  char buff[] = "abc";

  auto write_res = empty_file->write(0, strlen(buff), buff);
  ASSERT_FALSE(write_res);
  ASSERT_EQ(FileStoreError::FILE_CLOSED, write_res.error());

  auto read_res = empty_file->read(0, strlen(buff), buff);
  ASSERT_FALSE(read_res);
  ASSERT_EQ(FileStoreError::FILE_CLOSED, read_res.error());

  auto close_again_res = empty_file->close();
  ASSERT_FALSE(close_again_res);
  ASSERT_EQ(FileStoreError::FILE_CLOSED, close_again_res.error());

  auto res_size = empty_file->size();
  ASSERT_TRUE(res_size);
  ASSERT_EQ(0, res_size.value());
}

/**
 * @given file exists
 * @when try to write data to file
 * @then data is written and actual size written is returned
 */
TEST_F(FileSystemFileTest, WriteToFile) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);

  char buff[] = "hello world";
  size_t buff_size = strlen(buff);

  auto write_res = empty_file->write(0, buff_size, buff);

  ASSERT_TRUE(write_res);
  ASSERT_EQ(buff_size, write_res.value());

  auto check_file = std::ifstream(empty_file_path);
  char data_read[buff_size];
  check_file.read(data_read, buff_size);
  auto read_count = check_file.gcount();

  ASSERT_EQ(read_count, buff_size);
  ASSERT_TRUE(memcmp(buff, data_read, buff_size) == 0);

  // check file size
  auto size_res = empty_file->size();
  ASSERT_TRUE(size_res);
  ASSERT_EQ(buff_size, size_res.value());
}

/**
 * @given file exists
 * @when try to write data to file starting from pos
 * @then data is written and actual size written is returned
 */
TEST_F(FileSystemFileTest, WriteAtPos) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);

  size_t start = 12;
  char data[] = "hello world";
  size_t data_size = strlen(data);

  auto write_res = empty_file->write(start, data_size, data);
  ASSERT_TRUE(write_res);
  ASSERT_EQ(data_size, write_res.value());

  auto check_file = std::ifstream(empty_file_path);
  char data_read[start + data_size];
  check_file.read(data_read, start + data_size);
  auto read_count = check_file.gcount();

  // first *start* symbols are 0
  std::vector<char> zeroes(data_read, data_read + start);
  ASSERT_THAT(zeroes, testing::Each(0));

  ASSERT_EQ(read_count, start + data_size);
  ASSERT_TRUE(memcmp(data, data_read + start, data_size) == 0);

  // check file size
  auto size_res = empty_file->size();
  ASSERT_TRUE(size_res);
  ASSERT_EQ(start + data_size, size_res.value());
}

/**
 * @given file exists
 * @when overwrite old data from start position
 * @then data is written and actual size written is returned
 */
TEST_F(FileSystemFileTest, OverwriteAtPos) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);

  size_t start = 0;
  char data[] = "hello world";
  size_t data_size = strlen(data);

  auto write_res = empty_file->write(start, data_size, data);
  ASSERT_TRUE(write_res);
  ASSERT_EQ(data_size, write_res.value());

  start = 6;
  char more_data[] = "beauty C++ world";
  size_t more_data_size = strlen(more_data);

  write_res = empty_file->write(start, more_data_size, more_data);
  ASSERT_TRUE(write_res);
  ASSERT_EQ(more_data_size, write_res.value());

  char expected[] = "hello beauty C++ world";
  size_t expected_size = strlen(expected);
  auto check_file = std::ifstream(empty_file_path);
  char data_read[expected_size];
  check_file.read(data_read, expected_size);
  auto read_count = check_file.gcount();

  // first *start* symbols are 0
  ASSERT_EQ(read_count, expected_size);
  ASSERT_TRUE(memcmp(expected, data_read, expected_size) == 0);

  // check file size
  auto size_res = empty_file->size();
  ASSERT_TRUE(size_res);
  ASSERT_EQ(expected_size, size_res.value());
}

/**
 * @given empty file
 * @when try to read
 * @then read call is successfull, 0 bytes read
 */
TEST_F(FileSystemFileTest, ReadEmptyFile) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);

  char data[32];
  auto read_res = empty_file->read(0, 12, data);

  ASSERT_TRUE(read_res);
  ASSERT_EQ(0, read_res.value());
}

/**
 * @given open file with string "Hello C++ world"
 * @when read 3 chars from position 6
 * @then substring "C++" is read
 */
TEST_F(FileSystemFileTest, ReadFileFrom) {
  auto open_res = empty_file->open();
  ASSERT_TRUE(open_res);

  std::ofstream file(empty_file_path);
  file << "Hello C++ world";
  file.flush();

  size_t read_from = 6;
  size_t read_size = 3;
  char expected[] = "C++";
  char data_read[32];
  auto read_res = empty_file->read(read_from, read_size, data_read);
  ASSERT_TRUE(read_res);
  ASSERT_EQ(read_size, read_res.value());
  ASSERT_TRUE(memcmp(expected, data_read, read_size) == 0);
}
