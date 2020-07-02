
#include "sector_storage/stores/store_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, StoreErrors, e) {
  using fc::sector_storage::stores::StoreErrors;
  switch (e) {
    case (StoreErrors::FIND_AND_ALLOCATE):
      return "Store: can't both find and allocate a sector";
    case (StoreErrors::NOT_FOUND_PATH):
      return "Store: couldn't find a suitable path for a sector";
    case (StoreErrors::NOT_FOUND_STORAGE):
      return "Store: couldn't find the storage";
    case (StoreErrors::INVALID_SECTOR_NAME):
      return "Store: invalid sector file name";
    case (StoreErrors::INVALID_STORAGE_CONFIG):
      return "Store: can not open or parse the storage config";
    case (StoreErrors::CANNOT_CREATE_DIR):
      return "Store: can not create a directory";
    case (StoreErrors::DUPLICATE_STORAGE):
      return "Store: the storage is a duplicate";
    case (StoreErrors::NOT_FOUND_SECTOR):
      return "Store: couldn't find the sector";
    case (StoreErrors::CANNOT_REMOVE_SECTOR):
      return "Store: cannot remove the sector";
    case (StoreErrors::REMOVE_SEVERAL_FILE_TYPES):
      return "Store: remove expects one file type";
    case (StoreErrors::CANNOT_MOVE_SECTOR):
      return "Store: cannot move the sector";
    case (StoreErrors::CANNOT_INIT_LOGGER):
      return "Store: cannot init logger";
    case (StoreErrors::NO_REMOTE_STORAGE_URLS):
      return "Store: no known URLs for remote storage";
    case (StoreErrors::INVALID_URL):
      return "Store: invalid url of storage";
    case (StoreErrors::INVALID_FS_STAT_RESPONSE):
      return "Store: response with FS stat is invalid";
    case (StoreErrors::INTERNAL_SEVER_ERROR):
      return "Store: 500 response code received";
    case (StoreErrors::UNABLE_CREATE_REQUEST):
      return "Store: unable to create a response";
    case (StoreErrors::NOT_OK_STATUS_CODE):
      return "Store: non 200 response code received";
    case (StoreErrors::UNABLE_REMOTE_ACQUIRE_SECTOR):
      return "Store: failed to acquire sector from remote";
    case (StoreErrors::CANNOT_OPEN_TEMP_FILE):
      return "Store: cannot open temp file for downloading";
    case (StoreErrors::UNKNOWN_CONTENT_TYPE):
      return "Store: received unsupported content type";
    case (StoreErrors::CANNOT_REMOVE_OUTPUT_PATH):
      return "Store: cannot remove output path";
    case (StoreErrors::CANNOT_MOVE_FILE):
      return "Store: cannot move file";
    case (StoreErrors::NOT_FOUND_REQUESTED_SECTOR_TYPE):
      return "Store: Not found the requested type";
    default:
      return "Store: unknown error";
  }
}
