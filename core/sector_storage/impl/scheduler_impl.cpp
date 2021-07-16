/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"
#include <thread>
#include <utility>
#include "primitives/resources/active_resources.hpp"

namespace fc::sector_storage {
  using primitives::Resources;
  using primitives::WorkerResources;

  SchedulerImpl::SchedulerImpl(
      std::shared_ptr<boost::asio::io_context> io_context)
      : current_worker_id_(0),
        io_(std::move(io_context)),
        logger_(common::createLogger("scheduler")) {}

  outcome::result<void> SchedulerImpl::schedule(
      const primitives::sector::SectorRef &sector,
      const primitives::TaskType &task_type,
      const std::shared_ptr<WorkerSelector> &selector,
      const WorkerAction &prepare,
      const WorkerAction &work,
      const ReturnCb &cb,
      uint64_t priority) {
    std::shared_ptr<TaskRequest> request = std::make_shared<TaskRequest>(
        sector, task_type, priority, selector, prepare, work, cb);

    {
      std::lock_guard<std::mutex> lock(request_lock_);

      OUTCOME_TRY(scheduled, maybeScheduleRequest(request));

      if (!scheduled) {
        request_queue_.insert(request);
      }
    }

    return outcome::success();
  }

  void SchedulerImpl::newWorker(std::unique_ptr<WorkerHandle> worker) {
    std::unique_lock<std::mutex> lock(workers_lock_);
    if (current_worker_id_ == std::numeric_limits<uint64_t>::max()) {
      current_worker_id_ = 0;  // TODO: maybe better mechanism
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

    auto resource_iter = primitives::kResourceTable.find(
        {request->task_type, request->sector.proof_type});

    Resources need_resources{};
    if (resource_iter != primitives::kResourceTable.end()) {
      need_resources = resource_iter->second;
    }

    for (const auto &[wid, worker] : workers_) {
      OUTCOME_TRY(satisfies,
                  request->sel->is_satisfying(
                      request->task_type, request->sector.proof_type, worker));

      if (!satisfies) {
        continue;
      }
      tried++;

      if (!primitives::canHandleRequest(
              need_resources, worker->info.resources, worker->preparing)) {
        if (workers_.size() > 1 || active_jobs) {
          continue;
        }
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
    auto resource_iter = primitives::kResourceTable.find(
        {request->task_type, request->sector.proof_type});

    Resources need_resources{};
    if (resource_iter != primitives::kResourceTable.end()) {
      need_resources = resource_iter->second;
    }

    worker->preparing.add(worker->info.resources, need_resources);

    io_->post([this, wid, worker, request, need_resources]() {
      auto cb = [this, wid, worker, request, need_resources](
                    const outcome::result<CallResult> &res) -> void {
        bool force;
        {
          std::unique_lock<std::mutex> lock(workers_lock_);
          force = {workers_.size() == 1 && !active_jobs};
          ++active_jobs;
        }

        CallId call_id;

        auto maybe_clear = worker->active.withResources(
            force,
            worker->info.resources,
            need_resources,
            [&]() -> outcome::result<void> {
              worker->preparing.free(worker->info.resources, need_resources);

              auto maybe_call_id = request->work(worker->worker);

              if (maybe_call_id.has_value()) {
                call_id = maybe_call_id.value();
                return outcome::success();
              }

              return maybe_call_id.error();
            });

        if (maybe_clear.has_error()) {
          request->cb(maybe_clear.error());
          logger_->error("worker's execution: "
                         + maybe_clear.error().message());

          --active_jobs;
          freeWorker(wid);
        } else {
          ReturnCb new_cb =
              [this, wid, request, clear = std::move(maybe_clear.value())](
                  outcome::result<CallResult> result) -> void {
            clear();
            --active_jobs;

            request->cb(std::move(result));

            freeWorker(wid);
          };
          {
            std::unique_lock lock(cbs_lock_);

            auto it = results_.find(call_id);
            if (it == results_.end()) {
              callbacks_[call_id] = new_cb;
            } else {
              io_->post([cb = std::move(new_cb), value = it->second]() {
                cb(value);
              });
              results_.erase(it);
            }
          }
        }
      };

      if (not request->prepare) {
        return cb(outcome::success());
      }

      auto maybe_call_id = request->prepare(worker->worker);

      if (maybe_call_id.has_error()) {
        worker->preparing.free(worker->info.resources, need_resources);
        request->cb(maybe_call_id.error());
        freeWorker(wid);
        return;
      }

      {
        std::unique_lock lock(cbs_lock_);
        auto it = results_.find(maybe_call_id.value());
        if (it == results_.end()) {
          callbacks_[maybe_call_id.value()] = cb;
        } else {
          io_->post([cb = std::move(cb), value = it->second]() { cb(value); });
          results_.erase(it);
        }
      }
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
      auto maybe_satisfying = req->sel->is_satisfying(
          req->task_type, req->sector.proof_type, worker);
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
        req->cb(maybe_result.error());
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

  outcome::result<void> SchedulerImpl::returnResult(const CallId &call_id,
                                                    CallResult result) {
    std::unique_lock lock(cbs_lock_);

    auto it = callbacks_.find(call_id);
    if (it == callbacks_.end()) {
      results_[call_id] = result;
      return outcome::success();
    }

    io_->post([&, cb = it->second, call_id, result]() {
      cb(result);

      std::unique_lock lock(cbs_lock_);
      callbacks_.erase(call_id);
    });

    return outcome::success();
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
