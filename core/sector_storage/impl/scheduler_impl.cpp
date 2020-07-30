/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"
#include <boost/asio/post.hpp>
#include <boost/thread.hpp>
#include <thread>
#include "primitives/resources/active_resources.hpp"

namespace fc::sector_storage {
  using primitives::Resources;
  using primitives::WorkerResources;

  SchedulerImpl::SchedulerImpl(RegisteredProof seal_proof_type)
      : seal_proof_type_(seal_proof_type),
        current_worker_id_(0),
        logger_(common::createLogger("scheduler")) {
    unsigned int nthreads = 0;
    if ((nthreads = std::thread::hardware_concurrency())
        || (nthreads = boost::thread::hardware_concurrency())) {
      pool_ = std::make_unique<boost::asio::thread_pool>(nthreads);
    } else {
      pool_ = std::make_unique<boost::asio::thread_pool>(
          1);  // if we cannot get the number of cores, we think that the
               // processor has 1 core
    }
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

    {
      std::lock_guard<std::mutex> lock(request_lock_);

      OUTCOME_TRY(scheduled, maybeScheduleRequest(request));

      if (!scheduled) {
        request_queue_.insert(request);
      }
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

  void SchedulerImpl::newWorker(std::unique_ptr<WorkerHandle> worker) {
    std::unique_lock<std::mutex> lock(workers_lock_);
    if (current_worker_id_ == std::numeric_limits<uint64_t>::max()) {
      current_worker_id_ = 0;
    }
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

    Resources need_resources{};
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

      if (!primitives::canHandleRequest(
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
                           logger_->error("selecting best worker: "
                                          + maybe_res.error().message());
                           does_error_occurs = true;
                           return false;
                         }

                         return maybe_res.value();
                       });

      if (does_error_occurs) {
        return SchedulerErrors::kCannotSelectWorker;
      }

      WorkerID wid = acceptable[0];

      assignWorker(wid, workers_[wid], request);

      return true;
    }

    if (tried == 0) {
      return SchedulerErrors::kNotFoundWorker;
    }

    return false;
  }

  void SchedulerImpl::assignWorker(
      WorkerID wid,
      const std::shared_ptr<WorkerHandle> &worker,
      const std::shared_ptr<TaskRequest> &request) {
    auto resource_iter =
        primitives::kResourceTable.find({request->task_type, seal_proof_type_});

    Resources need_resources{};
    if (resource_iter != primitives::kResourceTable.end()) {
      need_resources = resource_iter->second;
    }

    worker->preparing.add(worker->info.resources, need_resources);

    boost::asio::post(*pool_, [this, wid, worker, request, need_resources]() {
      {
        auto maybe_err = request->prepare(worker->worker);
        std::unique_lock<std::mutex> lock(workers_lock_);
        if (maybe_err.has_error()) {
          worker->preparing.free(worker->info.resources, need_resources);
          request->respond(maybe_err.error());
          lock.unlock();
          freeWorker(wid);
          return;
        }

        maybe_err = worker->active.withResources(
            worker->info.resources,
            need_resources,
            workers_lock_,
            [&]() -> outcome::result<void> {
              worker->preparing.free(worker->info.resources, need_resources);
              lock.unlock();

              auto res = request->work(worker->worker);

              if (res.has_error()) {
                request->respond(res.error());
              } else {
                request->respond(std::error_code());
              }

              lock.lock();
              return outcome::success();
            });
        if (maybe_err.has_error()) {
          logger_->error("worker's execution: " + maybe_err.error().message());
        }
      }

      freeWorker(wid);
    });
  }

  void SchedulerImpl::freeWorker(WorkerID wid) {
    std::shared_ptr<WorkerHandle> worker;
    {
      std::lock_guard<std::mutex> lock(workers_lock_);
      auto iter = workers_.find(wid);
      if (iter == workers_.cend()) {
        logger_->warn("free worker: wid {} is invalid", wid);
        return;
      }
      worker = iter->second;
    }

    std::lock_guard<std::mutex> lock(request_lock_);
    for (auto it = request_queue_.begin(); it != request_queue_.cend(); ++it) {
      auto req = *it;
      auto maybe_satisfying =
          req->sel->is_satisfying(req->task_type, seal_proof_type_, worker);
      if (maybe_satisfying.has_error()) {
        logger_->error("free worker satisfactory check: "
                       + maybe_satisfying.error().message());
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
      if (it == request_queue_.cend()) {
        break;
      }
      --it;
    }
  }

  RegisteredProof SchedulerImpl::getSealProofType() const {
    return seal_proof_type_;
  }
}  // namespace fc::sector_storage

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, SchedulerErrors, e) {
  using fc::sector_storage::SchedulerErrors;
  switch (e) {
    case (SchedulerErrors::kCannotSelectWorker):
      return "Scheduler: some error occurred during select worker";
    case (SchedulerErrors::kNotFoundWorker):
      return "Scheduler: didn't find any good workers";
    default:
      return "Scheduler: unknown error";
  }
}
