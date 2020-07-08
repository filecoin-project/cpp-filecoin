/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_SCHEDULER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_SCHEDULER_HPP

#include "common/outcome.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "sector_storage/selector.hpp"

namespace fc::sector_storage {
  using primitives::TaskType;
  using primitives::sector::SectorId;

  using WorkerAction =
      std::function<outcome::result<void>(const std::shared_ptr<Worker> &)>;

  constexpr uint64_t kDefaultTaskPriority = 0;

  class Scheduler {
   public:
    virtual ~Scheduler() = default;

    virtual outcome::result<void> schedule(
        const SectorId &sector,
        const TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        uint64_t priority = kDefaultTaskPriority) = 0;

    virtual outcome::result<void> newWorker(
        std::unique_ptr<WorkerHandle> &&worker) = 0;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_SCHEDULER_HPP
