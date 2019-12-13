/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_FSLOCK_HPP
#define FILECOIN_CORE_FSLOCK_HPP

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <mutex>
#include <string>
#include "common/outcome.hpp"

namespace fc::fslock {
  outcome::result<boost::interprocess::file_lock> lock(
      const std::string lockFilePath);

  outcome::result<bool> isLocked(const std::string &lockFilePath);
}  // namespace fc::fslock

#endif  // FILECOIN_CORE_FSLOCK_HPP
