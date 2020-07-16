/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, StoreErrors, e) {
  using fc::sector_storage::stores::StoreErrors;
  switch (e) {
    case (StoreErrors::kFindAndAllocate):
      return "Store: can't both find and allocate a sector";
    case (StoreErrors::kNotFoundPath):
      return "Store: couldn't find a suitable path for a sector";
    case (StoreErrors::kNotFoundStorage):
      return "Store: couldn't find the storage";
    case (StoreErrors::kInvalidSectorName):
      return "Store: invalid sector file name";
    case (StoreErrors::kInvalidStorageConfig):
      return "Store: can not open or parse the storage config";
    case (StoreErrors::kCannotCreateDir):
      return "Store: can not create a directory";
    case (StoreErrors::kDuplicateStorage):
      return "Store: the storage is a duplicate";
    case (StoreErrors::kNotFoundSector):
      return "Store: couldn't find the sector";
    case (StoreErrors::kCannotRemoveSector):
      return "Store: cannot remove the sector";
    case (StoreErrors::kRemoveSeveralFileTypes):
      return "Store: remove expects one file type";
    case (StoreErrors::kCannotMoveSector):
      return "Store: cannot move the sector";
    case (StoreErrors::kCannotInitLogger):
      return "Store: cannot init logger";
    case (StoreErrors::kNoRemoteStorageUrls):
      return "Store: no known URLs for remote storage";
    case (StoreErrors::kInvalidUrl):
      return "Store: invalid url of storage";
    case (StoreErrors::kInvalidFsStatResponse):
      return "Store: response with FS stat is invalid";
    case (StoreErrors::kInternalServerError):
      return "Store: 500 response code received";
    case (StoreErrors::kUnableCreateRequest):
      return "Store: unable to create a response";
    case (StoreErrors::kNotOkStatusCode):
      return "Store: non 200 response code received";
    case (StoreErrors::kUnableRemoteAcquireSector):
      return "Store: failed to acquire sector from remote";
    case (StoreErrors::kCannotOpenTempFile):
      return "Store: cannot open temp file for downloading";
    case (StoreErrors::kUnknownContentType):
      return "Store: received unsupported content type";
    case (StoreErrors::kCannotRemoveOutputPath):
      return "Store: cannot remove output path";
    case (StoreErrors::kCannotMoveFile):
      return "Store: cannot move file";
    case (StoreErrors::kNotFoundRequestedSectorType):
      return "Store: Not found the requested type";
    default:
      return "Store: unknown error";
  }
}
