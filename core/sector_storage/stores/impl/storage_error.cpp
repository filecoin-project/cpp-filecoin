/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/storage_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, StorageError, e) {
  using fc::sector_storage::stores::StorageError;
  switch (e) {
    case (StorageError::kFileNotExist):
      return "Storage: the file doesn't exist";
    case StorageError::kFilesystemStatError:
      return "RepositoryError: cannot get statistic about proposed file";
    default:
      return "Storage: unknown error";
  }
}
