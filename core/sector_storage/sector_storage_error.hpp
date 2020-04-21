/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_ERROR_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::sector_storage {

  /**
   * @brief SectorStorage returns these types of errors
   */
  enum class SectorStorageError { CANNOT_CREATE_DIR = 1,
      UNABLE_ACCESS_SEALED_FILE,
      CANNOT_CLOSE_FILE,
      CANNOT_REMOVE_DIR,
      DONOT_MATCH_SIZES,
      UNKNOWN = 1000 };

}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, SectorStorageError);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_ERROR_HPP
