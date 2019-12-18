/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fslock/fslock_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::fslock, FSLockError, e) {
  using fc::fslock::FSLockError;

  switch (e) {
    case (FSLockError::FILE_LOCKED):
      return "FileLock: file is locked";
    case (FSLockError::UNKNOWN):
      return "FileLock: unknown error";
  }
}
