/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_SCHEDULER_IMPL_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_SCHEDULER_IMPL_HPP

#include "sector_storage/scheduler.hpp"

#include <mutex>
#include <unordered_map>
#include <utility>

namespace fc::sector_storage {
  using WorkerID = std::string;

  struct TaskRequest {
    inline TaskRequest(const SectorId &sector,
                       TaskType task_type,
                       uint64_t priority,
                       std::shared_ptr<WorkerSelector> sel,
                       WorkerAction prepare,
                       WorkerAction work)
        : sector(sector),
          task_type(std::move(task_type)),
          priority(priority),
          sel(std::move(sel)),
          prepare(std::move(prepare)),
          work(std::move(work)),
          response(boost::none){};

    SectorId sector;
    TaskType task_type;
    uint64_t priority;
    std::shared_ptr<WorkerSelector> sel;

    WorkerAction prepare;
    WorkerAction work;

    boost::optional<outcome::result<void>> response;
    std::mutex mutex_;
    std::condition_variable cv_;
  };

  class SchedulerImpl : public Scheduler {
   public:
    outcome::result<void> schedule(
        const SectorId &sector,
        const primitives::TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        uint64_t priority) override;

    outcome::result<void> newWorker(
        std::unique_ptr<WorkerHandle> &&worker) override;

   private:
    outcome::result<bool> maybeScheduleRequest(
        const std::shared_ptr<TaskRequest> &request);

    outcome::result<void> assignWorker(
        const std::pair<WorkerID, std::shared_ptr<WorkerHandle>> &worker,
        const TaskRequest &request);

    outcome::result<void> freeWorker(WorkerID wid);

    std::mutex workers_lock_;
    std::unordered_map<WorkerID, std::shared_ptr<WorkerHandle>> workers_;

    std::mutex request_lock_;
    std::multiset<std::shared_ptr<TaskRequest>,
                  std::owner_less<std::shared_ptr<TaskRequest>>>
        request_queue_;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_SCHEDULER_IMPL_HPP
