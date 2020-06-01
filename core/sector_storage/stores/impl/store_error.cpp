
#include "sector_storage/stores/store_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, StoreErrors, e) {
  using fc::sector_storage::stores::StoreErrors;
  switch (e) {
    case (StoreErrors::FindAndAllocate):
      return "Store: can't both find and allocate a sector";
      case (StoreErrors::NotFoundPath):
      return "Store: couldn't find a suitable path for a sector";
    default:
      return "Store: unknown error";
  }
}
