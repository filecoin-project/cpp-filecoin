/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"
#include "boost/thread/mutex.hpp"
#include "fslock/fslock_error.hpp"

namespace fc::fslock {
  outcome::result<boost::interprocess::file_lock> lock(
      const std::string lock_file_path) {
    auto lockFile = boost::interprocess::file_lock(lock_file_path.c_str());
    // TODO process error if file doesn't exist (FIL-41) artyom-yurin 12.12.2019
    if (!lockFile.try_lock()) {
      return FSLockError::FILE_LOCKED;
    }
    return std::move(lockFile);
  }

  outcome::result<bool> isLocked(const std::string &lockFilePath) {
    return outcome::success();
  }
}  // namespace fc::fslock
