/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"
#include <thread>
#include "primitives/resources/resources.hpp"

namespace fc::sector_storage {
  using primitives::Resources;
  using primitives::WorkerResources;

  bool canHandleRequest(const Resources &need_resources,
                        const WorkerResources &resources,
                        const ActiveResources &active) {
    // TODO: dedupe needRes.BaseMinMemory per task type
    // (don't add if that task is already running)
    uint64_t min_need_memory =
        resources.reserved_memory + active.memory_used_min
        + need_resources.min_memory + need_resources.base_min_memory;
    if (min_need_memory > resources.physical_memory) {
      return false;
    }

    uint64_t max_need_memory =
        resources.reserved_memory + active.memory_used_min
        + need_resources.max_memory + need_resources.base_min_memory;
    if (max_need_memory > (resources.swap_memory + resources.physical_memory)) {
      return false;
    }

    if (need_resources.threads) {
      if ((active.cpu_use + *need_resources.threads) > resources.cpus) {
        return false;
      }
    } else {
      if (active.cpu_use > 0) {
        return false;
      }
    }

    if (!resources.gpus.empty() && need_resources.can_gpu) {
      if (active.gpu_used) {
        return false;
      }
    }

    return true;
  }

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
    std::lock_guard<std::mutex> lock(workers_lock_);

    std::vector<WorkerID> acceptable;
    uint64_t tried = 0;

    auto resource_iter =
        primitives::kResourceTable.find({request->task_type, seal_proof_type_});

    Resources need_resources;
    if (resource_iter != primitives::kResourceTable.end()) {
      need_resources = resource_iter->second;
    }

    for (const auto &[wid, worker] : workers_) {
      OUTCOME_TRY(satisfies,
                  request->sel->is_satisfying(
                      request->task_type, seal_proof_type_, worker));

      if (!satisfies) {
        continue;
      }
      tried++;

      if (!canHandleRequest(
              need_resources, worker->info.resources, worker->preparing)) {
        continue;
      }

      acceptable.push_back(wid);
    }

    if (!acceptable.empty()) {
      bool does_error_occurs = false;
      std::stable_sort(acceptable.begin(),
                       acceptable.end(),
                       [&](WorkerID lhs, WorkerID rhs) {
                         auto maybe_res = request->sel->is_preferred(
                             request->task_type, workers_[lhs], workers_[rhs]);

                         if (maybe_res.has_error()) {
                           // TODO: Log it
                           does_error_occurs = true;
                           return false;
                         }

                         return maybe_res.value();
                       });

      if (does_error_occurs) {
        return outcome::success();  // TODO: ERROR
      }

      WorkerID wid = acceptable[0];

      std::thread t(
          &SchedulerImpl::assignWorker, this, wid, workers_[wid], request);
      t.detach();  // TODO: Make it via boost::asio

      return true;
    }

    if (tried == 0) {
      return outcome::success();  // TODO: ERROR
    }

    return false;
  }

  void SchedulerImpl::assignWorker(
      WorkerID wid,
      const std::shared_ptr<WorkerHandle> &worker,
      const std::shared_ptr<TaskRequest> &request) {
    // TODO: get resources

    // TODO: prepare

    // TODO: lock

    // TODO: if error. Send Err and Free Worker

    // TODO: run with resources

    // TODO: check that everthing is correct
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
