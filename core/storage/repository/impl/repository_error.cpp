/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/repository_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::repository, RepositoryError, e) {
  using fc::storage::repository::RepositoryError;

  switch (e) {
    case RepositoryError::kWrongVersion:
      return "RepositoryError: wrong version";
    case RepositoryError::kOpenFileError:
      return "RepositoryError: cannot open file";
    case RepositoryError::kInvalidStorageConfig:
      return "RepositoryError: cannot open or parse the storage config";
    case RepositoryError::kParseJsonError:
      return "RepositoryError: cannot parse .json config file";
    case RepositoryError::kWriteJsonError:
      return "RepositoryError: cannot write to the .json file";
    case RepositoryError::kTempDirectoryCreationError:
      return "RepositoryError: cannot create temporary directory";
    case RepositoryError::kFilesystemStatError:
      return "RepositoryError: cannot get statistic about proposed file";
    default:
      return "RepositoryError: unknown error";
  }
}
