/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_FSLOCK_FSLOCK_ERROR_HPP
#define FILECOIN_CORE_FSLOCK_FSLOCK_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::fslock {

  /**
   * @brief FSLock returns these types of errors
   */
  enum class FSLockError {
    FILE_NOT_FOUND = 1,
    FILE_LOCKED = 2,
    NO_RESOURCES = 3,

    UNKNOWN = 1000
  };

}  // namespace fc::fslock

OUTCOME_HPP_DECLARE_ERROR(fc::fslock, FSLockError);

#endif  // FILECOIN_CORE_FSLOCK_FSLOCK_ERROR_HPP
