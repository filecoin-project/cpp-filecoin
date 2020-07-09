/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_SCHEDULER_IMPL_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_SCHEDULER_IMPL_HPP

#include "sector_storage/scheduler.hpp"

#include <shared_mutex>
#include <unordered_map>

namespace fc::sector_storage {
  using WorkerID = std::string;

  struct TaskRequest {
    SectorId sector;
    TaskType task_type;
    uint64_t priority;
    std::shared_ptr<WorkerSelector> sel;

    WorkerAction prepare;
    WorkerAction work;
  };

  class SchedulerImpl : public Scheduler {
   public:
    outcome::result<void> schedule(
        const SectorId &sector,
        const TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        uint64_t priority) override;

    outcome::result<void> newWorker(
        std::unique_ptr<WorkerHandle> &&worker) override;

   private:
    outcome::result<bool> maybeScheduleRequest(const TaskRequest &request);

    outcome::result<void> assignWorker(
        const std::pair<WorkerID, std::shared_ptr<WorkerHandle>> &worker,
        const TaskRequest &request);

    outcome::result<void> freeWorker(WorkerID wid);

    std::mutex workers_lock_;
    std::unordered_map<WorkerID, std::shared_ptr<WorkerHandle>> workers_;

    std::mutex request_lock_;
    std::multiset<TaskRequest> request_queue_;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_SCHEDULER_IMPL_HPP
