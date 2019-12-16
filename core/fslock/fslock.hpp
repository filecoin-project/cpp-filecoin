/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_FSLOCK_HPP
#define FILECOIN_CORE_FSLOCK_HPP

#include <boost/interprocess/sync/file_lock.hpp>
#include <string>
#include "common/outcome.hpp"

namespace fc::fslock {

  /**
   * @brief tries to lock file
   * @param file_lock_path is path to file that we want to lock
   * @return lock file
   */
  outcome::result<boost::interprocess::file_lock> lock(
      const std::string &file_lock_path);
}  // namespace fc::fslock

#endif  // FILECOIN_CORE_FSLOCK_HPP
