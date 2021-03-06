/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::fslock {

  /**
   * @brief FSLock returns these types of errors
   */
  enum class FSLockError {
    kFileLocked = 1,
    kIsDirectory,
    kUnknown = 1000,
  };

}  // namespace fc::fslock

OUTCOME_HPP_DECLARE_ERROR(fc::fslock, FSLockError);
