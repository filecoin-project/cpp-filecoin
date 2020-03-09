/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/fslock/fslock_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::fslock, FSLockError, e) {
  using fc::fslock::FSLockError;

  switch (e) {
    case (FSLockError::FILE_LOCKED):
      return "FileLock: file is locked";
    case (FSLockError::IS_DIRECTORY):
      return "FileLock: cannot lock directory";
    case (FSLockError::UNKNOWN):
      return "FileLock: unknown error";
  }
}
