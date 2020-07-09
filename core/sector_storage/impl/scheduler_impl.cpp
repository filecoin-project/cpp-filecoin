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

  void SchedulerImpl::newWorker(std::unique_ptr<WorkerHandle> &&worker) {
    std::unique_lock<std::mutex> lock(workers_lock_);
    WorkerID wid = current_worker_id_++;
    workers_.insert({wid, std::move(worker)});
    lock.unlock();

    freeWorker(wid);
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

  void SchedulerImpl::freeWorker(WorkerID wid) {
    std::shared_ptr<WorkerHandle> worker;
    {
      std::lock_guard<std::mutex> lock(workers_lock_);
      auto iter = workers_.find(wid);
      if (iter == workers_.cend()) {
        // TODO: Log it
        return;
      }
    }

    std::lock_guard<std::mutex> lock(request_lock_);
    for (auto it = request_queue_.begin(); it != request_queue_.cend(); ++it) {
      auto req = *it;
      auto maybe_satisfying =
          req->sel->is_satisfying(req->task_type, seal_proof_type_, worker);
      if (maybe_satisfying.has_error()) {
        // TODO: Log it
        continue;
      }

      if (!maybe_satisfying.value()) {
        continue;
      }

      auto maybe_result = maybeScheduleRequest(req);

      if (maybe_result.has_error()) {
        req->respond(maybe_result.error());
      } else if (!maybe_result.value()) {
        continue;
      }

      it = request_queue_.erase(it);
      --it;
    }
  }
}  // namespace fc::sector_storage
