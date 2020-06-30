
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
    case (StoreErrors::NotFoundRequestedSectorType):
      return "Store: Not found the requested type";
    default:
      return "Store: unknown error";
  }
}
