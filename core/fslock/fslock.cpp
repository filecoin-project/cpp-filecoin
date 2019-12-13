/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include "fslock/fslock_error.hpp"

namespace fc::fslock {
  outcome::result<boost::interprocess::file_lock> lock(
      const std::string lock_file_path) {
    boost::filesystem::ofstream ofs(lock_file_path.c_str(), std::ios::app);
    ofs.close();
    auto lock_file = boost::interprocess::file_lock(lock_file_path.c_str());

    if (!lock_file.try_lock()) {
      return FSLockError::FILE_LOCKED;
    }
    return std::move(lock_file);
  }

  outcome::result<bool> isLocked(const std::string &lock_file_path) {
    if (!boost::filesystem::exists(lock_file_path)){
      return FSLockError::FILE_NOT_FOUND;
    }

    auto result = lock(lock_file_path);

    if (result.has_value()){
      result.value().unlock();
      return false;
    }

    return true;
  }
}  // namespace fc::fslock
