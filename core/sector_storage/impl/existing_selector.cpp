/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/existing_selector.hpp"

#include <unordered_set>

namespace fc::sector_storage {
  outcome::result<std::unique_ptr<WorkerSelector>>
  ExistingSelector::newExistingSelector(
      const std::shared_ptr<stores::SectorIndex> &index,
      const SectorId &sector,
      SectorFileType allocate,
      bool allow_fetch) {
    OUTCOME_TRY(best, index->storageFindSector(sector, allocate, allow_fetch));

    struct make_unique_enabler : public ExistingSelector {
      explicit make_unique_enabler(std::vector<stores::StorageInfo> best)
          : ExistingSelector{std::move(best)} {};
    };

    std::unique_ptr<ExistingSelector> selector =
        std::make_unique<make_unique_enabler>(std::move(best));

    return std::move(selector);
  }

  outcome::result<bool> ExistingSelector::is_satisfying(
      const TaskType &task,
      RegisteredProof seal_proof_type,
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

    for (const auto &info : best_) {
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

  ExistingSelector::ExistingSelector(std::vector<stores::StorageInfo> best)
      : best_(std::move(best)) {}
}  // namespace fc::sector_storage
