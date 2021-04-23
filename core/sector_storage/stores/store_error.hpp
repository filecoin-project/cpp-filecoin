/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORE_ERROR_HPP
#define CPP_FILECOIN_STORE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::sector_storage::stores {
  enum class StoreError {
    kFindAndAllocate = 1,
    kNotFoundPath,
    kNotFoundStorage,
    kInvalidSectorName,
    kInvalidStorageConfig,
    kCannotCreateDir,
    kDuplicateStorage,
    kNotFoundSector,
    kRemoveSeveralFileTypes,
    kCannotMoveSector,
    kNoRemoteStorageUrls,
    kInvalidUrl,
    kInvalidFsStatResponse,
    kInternalServerError,
    kUnableCreateRequest,
    kNotOkStatusCode,
    kUnableRemoteAcquireSector,
    kCannotOpenTempFile,
    kUnknownContentType,
    kCannotRemovePath,
    kCannotMoveFile,
    kCannotReserve,
    kAlreadyReserved,
    kConfigFileNotExist,
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, StoreError);

#endif  // CPP_FILECOIN_STORE_ERROR_HPP
