
#include "sector_storage/stores/store_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, StoreErrors, e) {
  using fc::sector_storage::stores::StoreErrors;
  switch (e) {
    case (StoreErrors::FindAndAllocate):
      return "Store: can't both find and allocate a sector";
    case (StoreErrors::NotFoundPath):
      return "Store: couldn't find a suitable path for a sector";
    case (StoreErrors::NotFoundStorage):
      return "Store: couldn't find the storage";
    case (StoreErrors::InvalidSectorName):
      return "Store: invalid sector file name";
    case (StoreErrors::InvalidStorageConfig):
      return "Store: can not open or parse the storage config";
    case (StoreErrors::CannotCreateDir):
      return "Store: can not create a directory";
    case (StoreErrors::DuplicateStorage):
      return "Store: the storage is a duplicate";
    case (StoreErrors::NotFoundSector):
      return "Store: couldn't find the sector";
    case (StoreErrors::CannotRemoveSector):
      return "Store: cannot remove the sector";
    case (StoreErrors::RemoveSeveralFileTypes):
      return "Store: remove expects one file type";
    case (StoreErrors::CannotMoveSector):
      return "Store: cannot move the sector";
    case (StoreErrors::CannotInitLogger):
      return "Store: cannot init logger";
    case (StoreErrors::NoRemoteStorageURLs):
      return "Store: no known URLs for remote storage";
    case (StoreErrors::InvalidUrl):
      return "Store: invalid url of storage";
    case (StoreErrors::InvalidFsStatResponse):
      return "Store: response with FS stat is invalid";
    case (StoreErrors::InternalServerError):
      return "Store: 500 response code received";
    case (StoreErrors::UnableCreateRequest):
      return "Store: unable to create a response";
    case (StoreErrors::NotOkStatusCode):
      return "Store: non 200 response code received";
    case (StoreErrors::UnableRemoteAcquireSector):
      return "Store: failed to acquire sector from remote";
    case (StoreErrors::CannotOpenTempFile):
      return "Store: cannot open temp file for downloading";
    case (StoreErrors::UnknownContentType):
      return "Store: received unsupported content type";
    case (StoreErrors::CannotRemoveOutputPath):
      return "Store: cannot remove output path";
    case (StoreErrors::CannotMoveFile):
      return "Store: cannot move file";
    default:
      return "Store: unknown error";
  }
}
