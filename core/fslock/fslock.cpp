/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include "fslock/fslock_error.hpp"

namespace fc::fslock {
  // TODO: Should be unlocked if process died
  outcome::result<boost::interprocess::file_lock> lock(
      const std::string &file_lock_path) {
    try {
      if (!boost::filesystem::exists(file_lock_path)) {
        boost::filesystem::ofstream os(file_lock_path);
        os.close();
      }
      boost::interprocess::file_lock file_lock(file_lock_path.c_str());
      if (!file_lock.try_lock()) {
        return FSLockError::FILE_LOCKED;
      }
      return std::move(file_lock);
    } catch (std::exception &) {
      return FSLockError::UNKNOWN;
    }
  }

}  // namespace fc::fslock
