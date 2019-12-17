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
  FSLockTest() : test::BaseFS_Test("/tmp/fc_filesystem_lock_file_test/") {}

  /** Path to lock file */
  Path lock_file_path;

  Path not_exist_file_path;

  int random_file_name_size_;

  /**
   * Create a test directory with an lock file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
    lock_file_path = fs::canonical(createFile("test.lock")).string();
    random_file_name_size_ = 5;
    std::string alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string rand_name = "";
    for (int i = 0; i < random_file_name_size_; i++) {
      rand_name += alpha[rand() % (alpha.length() - 1)];
    }
    not_exist_file_path = fs::current_path().string() + rand_name + ".lock";
  }
};

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
