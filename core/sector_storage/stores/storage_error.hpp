/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORES_STORAGE_ERROR_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORES_STORAGE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::sector_storage::stores {

  enum class StorageError {
    kFileNotExist = 1,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, StorageError);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORES_STORAGE_ERROR_HPP
