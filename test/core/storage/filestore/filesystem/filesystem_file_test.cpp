/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/filestore/impl/filesystem/filesystem_file.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "filecoin/storage/filestore/filestore_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::filestore::File;
using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::Path;

class FileSystemFileTest : public test::BaseFS_Test {
 public:
  FileSystemFileTest() : test::BaseFS_Test("fc_filesystem_file_test") {}

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
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, res.error());
  ASSERT_FALSE(file->is_open());

  auto res_size = file->size();
  ASSERT_FALSE(res_size);

  std::vector<uint8_t> buff{'a', 'b', 'c'};
  auto write_res = file->write(0, buff);
  ASSERT_FALSE(write_res);
  ASSERT_EQ(FileStoreError::FILE_NOT_FOUND, write_res.error());

  std::array<uint8_t, 3> read_buff{};
  auto read_res = file->read(0, read_buff);
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
  EXPECT_OUTCOME_TRUE_1(empty_file->open());
  ASSERT_TRUE(empty_file->is_open());
  ASSERT_EQ(empty_file_path, empty_file->path());

  // open again
  auto open_res = empty_file->open();
  ASSERT_FALSE(open_res);
  ASSERT_EQ(FileStoreError::CANNOT_OPEN, open_res.error());
  ASSERT_TRUE(empty_file->is_open());
}

/**
 * @given file is opened
 * @when close file is called
 * @then error FILE_CLOSED returned on method calls
 */
TEST_F(FileSystemFileTest, CloseFile) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  EXPECT_OUTCOME_TRUE_1(empty_file->close());
  ASSERT_FALSE(empty_file->is_open());

  std::vector<uint8_t> buff{'A', 'B', 'C'};
  auto write_res = empty_file->write(0, buff);
  ASSERT_FALSE(write_res);
  ASSERT_EQ(FileStoreError::FILE_CLOSED, write_res.error());

  std::array<uint8_t, 3> read_buff{};
  auto read_res = empty_file->read(0, read_buff);
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
 * @given an existing file
 * @when try to write 0 bytes to file
 * @then 0 size written is returned
 */
TEST_F(FileSystemFileTest, WriteZeroBytesToFile) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  std::vector<uint8_t> buff;
  EXPECT_OUTCOME_TRUE(write_res, empty_file->write(0, buff));
  ASSERT_EQ(0, write_res);
}

/**
 * @given file exists
 * @when try to write data to file
 * @then data is written and actual size written is returned
 */
TEST_F(FileSystemFileTest, WriteToFile) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  std::vector<uint8_t> buff{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
  const size_t buff_size = buff.size();

  EXPECT_OUTCOME_TRUE(write_res, empty_file->write(0, buff));
  ASSERT_EQ(buff_size, write_res);

  auto check_file = std::ifstream(empty_file_path);
  char data_read[buff_size];
  check_file.read(data_read, buff_size);
  auto read_count = check_file.gcount();

  ASSERT_EQ(read_count, buff_size);
  ASSERT_TRUE(memcmp(buff.data(), data_read, buff_size) == 0);

  // check file size
  EXPECT_OUTCOME_TRUE(size_res, empty_file->size());
  ASSERT_EQ(buff_size, size_res);
}

/**
 * @given file exists
 * @when try to write data to file starting from pos
 * @then data is written and actual size written is returned
 */
TEST_F(FileSystemFileTest, WriteAtPos) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  size_t start = 12;
  std::vector<uint8_t> data{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
  size_t data_size = data.size();

  EXPECT_OUTCOME_TRUE(write_res, empty_file->write(start, data));
  ASSERT_EQ(data_size, write_res);

  auto check_file = std::ifstream(empty_file_path);
  char data_read[start + data_size];
  check_file.read(data_read, start + data_size);
  auto read_count = check_file.gcount();

  // first *start* symbols are 0
  std::vector<uint8_t> zeroes(data_read, data_read + start);
  ASSERT_THAT(zeroes, testing::Each(0));

  ASSERT_EQ(read_count, start + data_size);
  ASSERT_TRUE(memcmp(data.data(), data_read + start, data_size) == 0);

  // check file size
  EXPECT_OUTCOME_TRUE(size_res, empty_file->size());
  ASSERT_EQ(start + data_size, size_res);
}

/**
 * @given file exists
 * @when overwrite old data from start position
 * @then data is written and actual size written is returned
 */
TEST_F(FileSystemFileTest, OverwriteAtPos) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  size_t start = 0;
  std::vector<uint8_t> data{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};

  EXPECT_OUTCOME_TRUE(write_res, empty_file->write(start, data));
  ASSERT_EQ(data.size(), write_res);

  start = 6;
  std::vector<uint8_t> more_data{'C', '+', '+', ' ', 'w', 'o', 'r', 'l', 'd'};

  EXPECT_OUTCOME_TRUE(write_res2, empty_file->write(start, more_data));
  ASSERT_EQ(more_data.size(), write_res2);

  std::vector<uint8_t> expected{'h',
                             'e',
                             'l',
                             'l',
                             'o',
                             ' ',
                             'C',
                             '+',
                             '+',
                             ' ',
                             'w',
                             'o',
                             'r',
                             'l',
                             'd'};
  const size_t expected_size = expected.size();
  auto check_file = std::ifstream(empty_file_path);
  char data_read[expected_size];
  check_file.read(data_read, expected_size);
  auto read_count = check_file.gcount();

  // first *start* symbols are 0
  ASSERT_EQ(read_count, expected_size);
  ASSERT_TRUE(memcmp(expected.data(), data_read, expected_size) == 0);

  // check file size
  auto size_res = empty_file->size();
  ASSERT_TRUE(size_res);
  ASSERT_EQ(expected_size, size_res.value());
}

/**
 * @given empty file
 * @when try to read
 * @then read call is successful, 0 bytes read
 */
TEST_F(FileSystemFileTest, ReadEmptyFile) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  std::vector<uint8_t> data(32);
  EXPECT_OUTCOME_TRUE(read_res, empty_file->read(0, data));
  ASSERT_EQ(0, read_res);
}

/**
 * @given open file with string "Hello C++ world"
 * @when read 3 chars from position 6
 * @then substring "C++" is read
 */
TEST_F(FileSystemFileTest, ReadFileFrom) {
  EXPECT_OUTCOME_TRUE_1(empty_file->open());

  std::ofstream file(empty_file_path);
  file << "Hello C++ world";
  file.flush();

  size_t read_from = 6;
  size_t read_size = 3;
  char expected[]{'C', '+', '+'};
  std::vector<uint8_t> data_read(read_size);
  EXPECT_OUTCOME_TRUE(read_res, empty_file->read(read_from, data_read));
  ASSERT_EQ(read_size, read_res);
  ASSERT_TRUE(memcmp(expected, data_read.data(), read_size) == 0);
}
