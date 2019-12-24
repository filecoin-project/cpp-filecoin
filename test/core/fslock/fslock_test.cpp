/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"

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
  FSLockTest() : test::BaseFS_Test("fc_filesystem_lock_file_test") {}

  /** Path to lock file */
  Path lock_file_path;

  Path not_exist_file_path;

  /** Path to dir */
  Path dir_path;

  /**
   * Create a test directory with an lock file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
    lock_file_path = fs::canonical(createFile("test.lock")).string();
    auto path_model = fs::current_path().append("%%%%%");
    not_exist_file_path = boost::filesystem::unique_path(path_model).string();

    dir_path = fs::current_path().string();
  }
};

/**
 * @given path to file and one process lock it
 * @when another process tries to lock it
 * @then error FILE_LOCKED
 */
TEST_F(FSLockTest, LockFileSuccess) {
  EXPECT_OUTCOME_TRUE_1(fc::fslock::Locker::lock(lock_file_path));
  auto pid = fork();
  if (pid == 0) {
    EXPECT_OUTCOME_FALSE(err, fc::fslock::Locker::lock(lock_file_path));
    ASSERT_EQ(err, fc::fslock::FSLockError::FILE_LOCKED);
  } else {
    wait(NULL);
  }
}

/**
 * @given path to file that doesn't exist
 * @when one process create and lock file and another tries to lock it at the same time
 * @then the first get lock file and the second get error FILE_LOCKED
 */
TEST_F(FSLockTest, LockNotExistingFileSuccess) {
  auto pid = fork();
  if (pid == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_OUTCOME_FALSE(err, fc::fslock::Locker::lock(not_exist_file_path));
    ASSERT_EQ(err, fc::fslock::FSLockError::FILE_LOCKED);
  } else {
    EXPECT_OUTCOME_TRUE_1(fc::fslock::Locker::lock(not_exist_file_path));
    wait(NULL);
  }
}

/**
 * @given path to directory
 * @when process tries to lock it
 * @then error IS_DIRECTORY
 */
TEST_F(FSLockTest, LockDirectoryFail) {
  EXPECT_OUTCOME_FALSE(err1,fc::fslock::Locker::lock(dir_path));
  ASSERT_EQ(err1, fc::fslock::FSLockError::IS_DIRECTORY);
}
