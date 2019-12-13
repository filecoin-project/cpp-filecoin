/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include "fslock/fslock_error.hpp"
#include "storage/filestore/impl/filesystem/filesystem_file.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::filestore::File;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::Path;

class FSLockTest : public test::BaseFS_Test {
 public:
  FSLockTest() : test::BaseFS_Test("/tmp/fc_filesystem_lock_file_test/") {}

  /** Path to lock file */
  Path lock_file_path;

  Path not_exist_file_path;

  /**
   * Create a test directory with an lock file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
    lock_file_path = fs::canonical(createFile("test.lock")).string();
    not_exist_file_path = "/tmp/fc_filesystem_lock_file_test/not_file.lock";
  }
};

TEST_F(FSLockTest, LockFile) {
  EXPECT_OUTCOME_TRUE(res, fc::fslock::lock(lock_file_path));
  auto pid = fork();
  if (pid == 0) {
    EXPECT_OUTCOME_FALSE(err, fc::fslock::lock(lock_file_path));
    ASSERT_EQ(err, fc::fslock::FSLockError::FILE_LOCKED);
  } else {
    wait(NULL);
  }
}

TEST_F(FSLockTest, NoExistLockFile) {
  EXPECT_OUTCOME_TRUE(val, fc::fslock::lock(not_exist_file_path));
  auto pid = fork();
  if (pid == 0) {
    EXPECT_OUTCOME_FALSE(err, fc::fslock::lock(not_exist_file_path));
    ASSERT_EQ(err, fc::fslock::FSLockError::FILE_LOCKED);
  } else {
    wait(NULL);
  }
}
