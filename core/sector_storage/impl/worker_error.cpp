/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/worker.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, WorkerErrors, e) {
  using fc::sector_storage::WorkerErrors;
  switch (e) {
    case (WorkerErrors::kCannotCreateSealedFile):
      return "Worker: cannot create sealed file";
    case (WorkerErrors::kCannotCreateCacheDir):
      return "Worker: cannot create cache dir";
    case (WorkerErrors::kCannotRemoveCacheDir):
      return "Worker: cannot remove cache dir";
    case (WorkerErrors::kPiecesDoNotMatchSectorSize):
      return "Worker: aggregated piece sizes don't match sector size";
    case (WorkerErrors::kCannotCreateTempFile):
      return "Worker: cannot create a temp file to place piece";
    case (WorkerErrors::kCannotGetNumberOfCPUs):
      return "Worker: cannot get number of cpus";
    case (WorkerErrors::kCannotGetVMStat):
      return "Worker: cannot get virtual memory statistic";
    case (WorkerErrors::kCannotGetPageSize):
      return "Worker: cannot get page size";
    case (WorkerErrors::kCannotOpenMemInfoFile):
      return "Worker: cannot open memory info file";
    case (WorkerErrors::kCannotRemoveSector):
      return "Worker: some errors occurred while removing sector";
    case (WorkerErrors::kUnsupportedPlatform):
      return "Worker: the platfrom not supported ";
    default:
      return "Worker: unknown error";
  }
}
