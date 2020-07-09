/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"

namespace fc::sector_storage {
  outcome::result<void> SchedulerImpl::schedule(
      const primitives::sector::SectorId &sector,
      const primitives::TaskType &task_type,
      const std::shared_ptr<WorkerSelector> &selector,
      const WorkerAction &prepare,
      const WorkerAction &work,
      uint64_t priority) {
    std::shared_ptr<TaskRequest> request = std::make_shared<TaskRequest>(
        sector, task_type, priority, selector, prepare, work);

    OUTCOME_TRY(scheduled, maybeScheduleRequest(request));

    if (!scheduled) {
      std::lock_guard<std::mutex> lock(request_lock_);
      request_queue_.insert(request);
    }

    while (true) {
      if (request->response) {
        break;
      }

      std::unique_lock<std::mutex> lock(request->mutex_);
      request->cv_.wait(lock, [&] { return request->response; });
    }

    return *request->response;
  }

  outcome::result<void> SchedulerImpl::newWorker(
      std::unique_ptr<WorkerHandle> &&worker) {
    return outcome::success();
  }

  outcome::result<bool> SchedulerImpl::maybeScheduleRequest(
      const std::shared_ptr<TaskRequest> &request) {
    return outcome::success();
  }

  outcome::result<void> SchedulerImpl::assignWorker(
      const std::pair<WorkerID, std::shared_ptr<WorkerHandle>> &worker,
      const TaskRequest &request) {
    return outcome::success();
  }

  outcome::result<void> SchedulerImpl::freeWorker(WorkerID wid) {
    return outcome::success();
  }
}  // namespace fc::sector_storage
