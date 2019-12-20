/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/repository_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::repository, RepositoryError, e) {
  using fc::storage::repository::RepositoryError;

  switch (e) {
    case RepositoryError::WRONG_VERSION:
      return "RepositoryError: wrong version";
    case RepositoryError::UNKNOWN:
      break;
  }

  return "unknown error";
}
