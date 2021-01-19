/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/existing_selector.hpp"

#include <unordered_set>
#include "primitives/types.hpp"

namespace fc::sector_storage {

  outcome::result<bool> ExistingSelector::is_satisfying(
      const TaskType &task,
      RegisteredSealProof seal_proof_type,
      const std::shared_ptr<WorkerHandle> &worker) {
    OUTCOME_TRY(tasks, worker->worker->getSupportedTask());
    if (tasks.find(task) == tasks.end()) {
      return false;
    }

    OUTCOME_TRY(paths, worker->worker->getAccessiblePaths());

    std::set<primitives::StorageID> storages = {};
    for (const auto &path : paths) {
      storages.insert(path.id);
    }

    OUTCOME_TRY(best,
                index_->storageFindSector(
                    sector_,
                    allocate_,
                    ((allow_fetch_) ? boost::make_optional(seal_proof_type)
                                    : boost::none)));
    for (const auto &info : best) {
      if (storages.find(info.id) != storages.end()) {
        return true;
      }
    }

    return false;
  }

  outcome::result<bool> ExistingSelector::is_preferred(
      const TaskType &task,
      const std::shared_ptr<WorkerHandle> &challenger,
      const std::shared_ptr<WorkerHandle> &current_best) {
    return challenger->active.utilization(challenger->info.resources)
           < current_best->active.utilization(current_best->info.resources);
  }

  ExistingSelector::ExistingSelector(std::shared_ptr<stores::SectorIndex> index,
                                     SectorId sector,
                                     SectorFileType allocate,
                                     bool allow_fetch)
      : index_(std::move(index)),
        sector_(sector),
        allocate_(allocate),
        allow_fetch_(allow_fetch) {}
}  // namespace fc::sector_storage
