/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORE_ERROR_HPP
#define CPP_FILECOIN_STORE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::sector_storage::stores {
  enum class StoreErrors {
    FIND_AND_ALLOCATE = 1,
    NOT_FOUND_PATH,
    NOT_FOUND_STORAGE,
    INVALID_SECTOR_NAME,
    INVALID_STORAGE_CONFIG,
    CANNOT_CREATE_DIR,
    DUPLICATE_STORAGE,
    NOT_FOUND_SECTOR,
    CANNOT_REMOVE_SECTOR,
    REMOVE_SEVERAL_FILE_TYPES,
    CANNOT_MOVE_SECTOR,
    CANNOT_INIT_LOGGER,
    NO_REMOTE_STORAGE_URLS,
    INVALID_URL,
    INVALID_FS_STAT_RESPONSE,
    INTERNAL_SEVER_ERROR,
    UNABLE_CREATE_REQUEST,
    NOT_OK_STATUS_CODE,
    UNABLE_REMOTE_ACQUIRE_SECTOR,
    CANNOT_OPEN_TEMP_FILE,
    UNKNOWN_CONTENT_TYPE,
    CANNOT_REMOVE_OUTPUT_PATH,
    CANNOT_MOVE_FILE,
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, StoreErrors);

#endif  // CPP_FILECOIN_STORE_ERROR_HPP
