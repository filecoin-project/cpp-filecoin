/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/allocate_selector.hpp"

#include <unordered_set>

namespace fc::sector_storage {

  outcome::result<bool> AllocateSelector::is_satisfying(
      const TaskType &task,
      RegisteredSealProof seal_proof_type,
      const std::shared_ptr<WorkerHandle> &worker) {
    OUTCOME_TRY(tasks, worker->worker->getSupportedTask());
    if (tasks.find(task) == tasks.end()) {
      return false;
    }

    OUTCOME_TRY(paths, worker->worker->getAccessiblePaths());

    std::unordered_set<primitives::StorageID> storages{};
    for (const auto &path : paths) {
      storages.insert(path.id);
    }

    OUTCOME_TRY(sector_size, getSectorSize(seal_proof_type));

    OUTCOME_TRY(best,
                sector_index_->storageBestAlloc(
                    allocate_, sector_size, path_type_ == PathType::kSealing));

    for (const auto &info : best) {
      if (storages.find(info.id) != storages.end()) {
        return true;
      }
    }

    return false;
  }

  outcome::result<bool> AllocateSelector::is_preferred(
      const TaskType &task,
      const std::shared_ptr<WorkerHandle> &challenger,
      const std::shared_ptr<WorkerHandle> &current_best) {
    return challenger->active.utilization(challenger->info.resources)
           < current_best->active.utilization(current_best->info.resources);
  }

  AllocateSelector::AllocateSelector(std::shared_ptr<stores::SectorIndex> index,
                                     SectorFileType allocate,
                                     PathType path_type)
      : sector_index_(std::move(index)),
        allocate_(allocate),
        path_type_(path_type) {}

}  // namespace fc::sector_storage
