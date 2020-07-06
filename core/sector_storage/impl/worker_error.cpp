/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/worker.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, WorkerErrors, e) {
  using fc::sector_storage::WorkerErrors;
  switch (e) {
    case (WorkerErrors::CANNOT_CREATE_SEALED_FILE):
      return "Worker: cannot create sealed file";
    case (WorkerErrors::CANNOT_CREATE_CACHE_DIR):
      return "Worker: cannot create cache dir";
    case (WorkerErrors::CANNOT_REMOVE_CACHE_DIR):
      return "Worker: cannot remove cache dir";
    case (WorkerErrors::PIECES_DO_NOT_MATCH_SECTOR_SIZE):
      return "Worker: aggregated piece sizes don't match sector size";
    case (WorkerErrors::OUTPUT_DOES_NOT_OPEN):
      return "Worker: output doesn't open to write piece";
    case (WorkerErrors::OUT_OF_BOUND_OF_FILE):
      return "Worker: requested piece is out of bound";
    case (WorkerErrors::CANNOT_OPEN_UNSEALED_FILE):
      return "Worker: cannot open unsealed file";
    case (WorkerErrors::CANNOT_GET_NUMBER_OF_CPUS):
      return "Worker: cannot get number of cpus";
    case (WorkerErrors::CANNOT_GET_VM_STAT):
      return "Worker: cannot get virtual memory statistic";
    case (WorkerErrors::CANNOT_GET_PAGE_SIZE):
      return "Worker: cannot get page size";
    case (WorkerErrors::CANNOT_OPEN_MEM_INFO_FILE):
      return "Worker: cannot open memory info file";
    case (WorkerErrors::CANNOT_REMOVE_SECTOR):
      return "Worker: some errors occurred while removing sector";
    default:
      return "Worker: unknown error";
  }
}
