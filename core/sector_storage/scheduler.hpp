/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "sector_storage/selector.hpp"
#include "sector_storage/worker.hpp"

namespace fc::sector_storage {
  using primitives::TaskType;
  using primitives::sector::SectorId;

  using WorkerAction =
      std::function<outcome::result<CallId>(const std::shared_ptr<Worker> &)>;

  constexpr uint64_t kDefaultTaskPriority = 0;

  class Scheduler {
   public:
    virtual ~Scheduler() = default;

    virtual outcome::result<void> schedule(
        const SectorRef &sector,
        const TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        const ReturnCb &cb,
        uint64_t priority = kDefaultTaskPriority) = 0;

    virtual void newWorker(std::unique_ptr<WorkerHandle> worker) = 0;

    virtual outcome::result<void> returnResult(const CallId &call_id,
                                               CallResult result) = 0;
  };

  enum class SchedulerErrors {
    kCannotSelectWorker = 0,
    kNotFoundWorker,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, SchedulerErrors);
