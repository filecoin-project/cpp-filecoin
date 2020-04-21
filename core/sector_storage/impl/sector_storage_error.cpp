/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/sector_storage_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, SectorStorageError, e) {
  using fc::sector_storage::SectorStorageError;

  switch (e) {
    case (SectorStorageError::CANNOT_CREATE_DIR):
      return "Sector Storage: cannot create a directory";
    case (SectorStorageError::UNABLE_ACCESS_SEALED_FILE):
      return "Sector Storage: unable to access the sealed file";
    case (SectorStorageError::CANNOT_CLOSE_FILE):
      return "Sector Storage: cannot close the file";
    case (SectorStorageError::CANNOT_REMOVE_DIR):
      return "Sector Storage: cannot remove a directory";
    case (SectorStorageError::DONOT_MATCH_SIZES):
      return "Sector Storage: aggregated piece sizes don't match sector size";
    default:
      return "Sector Storage: unknown error";
  }
}
