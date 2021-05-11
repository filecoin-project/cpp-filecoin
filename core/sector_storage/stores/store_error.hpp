/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::sector_storage::stores {
  enum class StoreErrors {
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
    kCannotInitLogger,
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
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, StoreErrors);
