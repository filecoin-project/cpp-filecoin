/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "testutil/storage/base_fs_test.hpp"
#include "storage/filestore/impl/filesystem/filesystem_file.hpp"
#include <thread>
#include "fslock/fslock_error.hpp"

using fc::storage::filestore::File;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::Path;

class FSLockTest : public test::BaseFS_Test {
 public:
  FSLockTest() : test::BaseFS_Test("/tmp/fc_filesystem_lock_file_test/") {}

  /** Path to lock file */
  Path lock_file_path;

  /** Lock file */
  std::shared_ptr<File> lock_file;

  /**
   * Create a test directory with an lock file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
    lock_file_path = fs::canonical(createFile("test.lock")).string();
    lock_file = std::make_shared<FileSystemFile>(lock_file_path);
  }
};

TEST_F(FSLockTest, LockFile){
  auto res = fc::fslock::lock(lock_file_path);
  ASSERT_TRUE(res.has_value());
  auto pid = fork();
  if (pid == 0){
    auto again = fc::fslock::lock(lock_file_path);
    ASSERT_TRUE(again.has_error());
    ASSERT_EQ(again.error(), fc::fslock::FSLockError::FILE_LOCKED);
  } else {
    wait(NULL);
  }
}
