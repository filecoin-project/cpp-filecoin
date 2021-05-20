/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, StoreError, e) {
  using fc::sector_storage::stores::StoreError;
  switch (e) {
    case (StoreError::kFindAndAllocate):
      return "Store: can't both find and allocate a sector";
    case (StoreError::kNotFoundPath):
      return "Store: couldn't find a suitable path for a sector";
    case (StoreError::kNotFoundStorage):
      return "Store: couldn't find the storage";
    case (StoreError::kInvalidSectorName):
      return "Store: invalid sector file name";
    case (StoreError::kInvalidStorageConfig):
      return "Store: can not open or parse the storage config";
    case (StoreError::kCannotCreateDir):
      return "Store: can not create a directory";
    case (StoreError::kDuplicateStorage):
      return "Store: the storage is a duplicate";
    case (StoreError::kNotFoundSector):
      return "Store: couldn't find the sector";
    case (StoreError::kRemoveSeveralFileTypes):
      return "Store: remove expects one file type";
    case (StoreError::kCannotMoveSector):
      return "Store: cannot move the sector";
    case (StoreError::kNoRemoteStorageUrls):
      return "Store: no known URLs for remote storage";
    case (StoreError::kInvalidUrl):
      return "Store: invalid url of storage";
    case (StoreError::kInvalidFsStatResponse):
      return "Store: response with FS stat is invalid";
    case (StoreError::kInternalServerError):
      return "Store: 500 response code received";
    case (StoreError::kUnableCreateRequest):
      return "Store: unable to create a response";
    case (StoreError::kNotOkStatusCode):
      return "Store: non 200 response code received";
    case (StoreError::kUnableRemoteAcquireSector):
      return "Store: failed to acquire sector from remote";
    case (StoreError::kCannotOpenTempFile):
      return "Store: cannot open temp file for downloading";
    case (StoreError::kUnknownContentType):
      return "Store: received unsupported content type";
    case (StoreError::kCannotRemovePath):
      return "Store: cannot remove path";
    case (StoreError::kCannotMoveFile):
      return "Store: cannot move file";
    case (StoreError::kCannotReserve):
      return "Store: not enough bytes. Cannot reserve";
    case (StoreError::kAlreadyReserved):
      return "Store: the type is already reserved";
    case (StoreError::kConfigFileNotExist):
      return "Store: config file doesn't exist";
    default:
      return "Store: unknown error";
  }
}
