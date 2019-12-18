/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include "fslock/fslock_error.hpp"

namespace fc::fslock {
  // TODO(artyom-yurin): [FIL-115] Should be unlocked if process died
  outcome::result<boost::interprocess::file_lock> Locker::lock(
      const std::string &file_lock_path) {
    try {
      boost::interprocess::scoped_lock scopedMutex(mutex);
      if (!boost::filesystem::exists(file_lock_path)) {
        boost::filesystem::ofstream os(file_lock_path);
        os.close();
      }
      boost::interprocess::file_lock file_lock(file_lock_path.c_str());
      if (!file_lock.try_lock()) {
        return FSLockError::FILE_LOCKED;
      }
      return std::move(file_lock);
    } catch (...) {
      return FSLockError::UNKNOWN;
    }
  }

  boost::interprocess::named_mutex Locker::mutex =
      boost::interprocess::named_mutex(boost::interprocess::open_or_create,
                                       "locker");
}  // namespace fc::fslock
