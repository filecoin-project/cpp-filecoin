/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"

#include <thread>
#include <utility>
#include "codec/cbor/cbor_codec.hpp"
#include "primitives/resources/active_resources.hpp"

namespace fc::sector_storage {
  using primitives::Resources;
  using primitives::WorkerResources;

  outcome::result<std::shared_ptr<SchedulerImpl>> SchedulerImpl::newScheduler(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<BufferMap> datastore,
      std::shared_ptr<Estimator> estimator) {
    struct make_unique_enabler : public SchedulerImpl {
      make_unique_enabler(std::shared_ptr<boost::asio::io_context> io_context,
                          std::shared_ptr<BufferMap> datastore,
                          std::shared_ptr<Estimator> estimator)
          : SchedulerImpl{std::move(io_context),
                          std::move(datastore),
                          std::move(estimator)} {};
    };

    std::shared_ptr<SchedulerImpl> scheduler =
        std::make_shared<make_unique_enabler>(
            std::move(io_context), std::move(datastore), std::move(estimator));

    OUTCOME_TRY(scheduler->resetWorks());

    return scheduler;
  }

  SchedulerImpl::SchedulerImpl(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<BufferMap> datastore,
      std::shared_ptr<Estimator> estimator)
      : current_worker_id_(0),
        estimator_(std::move(estimator)),
        call_kv_(std::move(datastore)),
        io_(std::move(io_context)),
        logger_(common::createLogger("scheduler")) {}

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> SchedulerImpl::schedule(
      const primitives::sector::SectorRef &sector,
      const primitives::TaskType &task_type,
      const std::shared_ptr<WorkerSelector> &selector,
      const WorkerAction &prepare,
      const WorkerAction &work,
      const ReturnCb &cb,
      uint64_t priority,
      const boost::optional<WorkId> &maybe_work_id) {
    WorkerAction job = work;
    ReturnCb callback = cb;
    if (maybe_work_id.has_value()) {
      const auto &work_id{maybe_work_id.value()};

      callback =
          [logger{logger_},
           old_cb = std::move(callback),
           kv{call_kv_},
           wid{work_id}](const outcome::result<CallResult> &result) -> void {
        old_cb(result);
        auto maybe_error = kv->remove(static_cast<Bytes>(wid));
        if (maybe_error.has_error()) {
          logger->error(maybe_error.error().message());
        }
      };

      if (call_kv_->contains(static_cast<Bytes>(work_id))) {
        // already in progress

        OUTCOME_TRY(raw_state, call_kv_->get(static_cast<Bytes>(work_id)));
        OUTCOME_TRY(state, codec::cbor::decode<WorkState>(raw_state));

        if (state.status == WorkStatus::kInProgress) {
          std::unique_lock lock(cbs_lock_);

          auto it = results_.find(state.call_id);
          if (it != results_.end()) {
            lock.unlock();
            callback(it->second);
            lock.lock();
            it = results_.find(state.call_id);
            results_.erase(it);
          } else {
            callbacks_[state.call_id] = callback;
          }
        }

        return outcome::success();
      }
      // new task
      WorkState state;
      state.id = work_id;
      state.status = WorkStatus::kStart;
      OUTCOME_TRY(state_raw, codec::cbor::encode(state));

      OUTCOME_TRY(
          call_kv_->put(static_cast<Bytes>(work_id), std::move(state_raw)));

      job = [old_job = std::move(job), kv{call_kv_}, wid{work_id}](
                const std::shared_ptr<Worker> &worker)
          -> outcome::result<CallId> {
        OUTCOME_TRY(call_id, old_job(worker));
        // check that the work running on a remote worker
        if (not worker->isLocalWorker()) {
          WorkState state;
          state.id = wid;
          state.status = WorkStatus::kInProgress;
          state.call_id = call_id;
          OUTCOME_TRY(state_raw, codec::cbor::encode(state));
          OUTCOME_TRY(kv->put(static_cast<Bytes>(wid), std::move(state_raw)));
        }
        return std::move(call_id);
      };
    }

    std::shared_ptr<TaskRequest> request =
        std::make_shared<TaskRequest>(sector,
                                      task_type,
                                      priority,
                                      selector,
                                      prepare,
                                      std::move(job),
                                      std::move(callback));

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
      current_worker_id_ = 0;  // TODO(ortyomka): maybe better mechanism
    }
    WorkerId wid = current_worker_id_++;
    workers_.insert({wid, std::move(worker)});
    lock.unlock();

    freeWorker(wid);
  }

  outcome::result<bool> SchedulerImpl::maybeScheduleRequest(
      const std::shared_ptr<TaskRequest> &request) {
    std::lock_guard<std::mutex> lock(workers_lock_);

    std::vector<WorkerId> acceptable;
    uint64_t tried = 0;

    for (const auto &[wid, worker] : workers_) {
      OUTCOME_TRY(satisfies,
                  request->sel->is_satisfying(
                      request->task_type, request->sector.proof_type, worker));

      if (!satisfies) {
        continue;
      }
      tried++;

      if (!worker->preparing.canHandleRequest(request->need_resources,
                                              worker->info.resources)) {
        if ((workers_.size() > 1) || (active_jobs != 0)) {
          continue;
        }
      }

      acceptable.push_back(wid);
    }

    if (!acceptable.empty()) {
      bool does_error_occurs = false;
      std::stable_sort(
          acceptable.begin(),
          acceptable.end(),
          [&](WorkerId lhs, WorkerId rhs) {
            const auto l_time = estimator_->getTime(lhs, request->task_type);
            const auto r_time = estimator_->getTime(rhs, request->task_type);

            // if time is available, then compare them
            if (l_time and r_time) {
              return l_time < r_time;
            }

            // if some workers don't have data about time, then we prefer worker
            // without time, to give a chance to prove yourself
            if (l_time) {
              return false;
            }

            if (r_time) {
              return true;
            }

            // if both workers without time, then compare with selector
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

      WorkerId wid = acceptable[0];

      assignWorker(wid, workers_[wid], request);

      return true;
    }

    if (tried == 0) {
      return SchedulerErrors::kNotFoundWorker;
    }

    return false;
  }

  void SchedulerImpl::assignWorker(
      WorkerId wid,
      const std::shared_ptr<WorkerHandle> &worker,
      const std::shared_ptr<TaskRequest> &request) {
    worker->preparing.add(worker->info.resources, request->need_resources);

    io_->post([this, wid, worker, request]() {
      auto cb = [this, wid, worker, request](
                    const outcome::result<CallResult> &res) -> void {
        ++active_jobs;

        worker->preparing.free(worker->info.resources, request->need_resources);

        worker->active.add(worker->info.resources, request->need_resources);

        auto clear = [this, wid, worker, request]() {
          worker->active.free(worker->info.resources, request->need_resources);
          --active_jobs;
          freeWorker(wid);
        };

        auto maybe_call_id = request->work(worker->worker);

        if (maybe_call_id.has_error()) {
          request->cb(maybe_call_id.error());
          logger_->error("worker's execution: "
                         + maybe_call_id.error().message());
          return clear();
        }
        estimator_->startWork(wid, request->task_type, maybe_call_id.value());
        ReturnCb new_cb = [call_id{maybe_call_id.value()},
                           estimator{estimator_},
                           request,
                           clear = std::move(clear)](
                              outcome::result<CallResult> result) -> void {
          if (result.has_value()) {
            estimator->finishWork(call_id);
          } else {
            estimator->abortWork(call_id);
          }

          request->cb(std::move(result));

          return clear();
        };
        auto &call_id{maybe_call_id.value()};
        std::unique_lock lock(cbs_lock_);

        auto it = results_.find(call_id);
        if (it == results_.end()) {
          callbacks_[call_id] = new_cb;
        } else {
          io_->post(
              [cb = std::move(new_cb), value = it->second]() { cb(value); });
          results_.erase(it);
        }
      };

      if (not request->prepare) {
        return cb(outcome::success());
      }

      auto maybe_call_id = request->prepare(worker->worker);

      if (maybe_call_id.has_error()) {
        worker->preparing.free(worker->info.resources, request->need_resources);
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

  void SchedulerImpl::freeWorker(WorkerId wid) {
    std::shared_ptr<WorkerHandle> worker;
    {
      std::lock_guard<std::mutex> lock(workers_lock_);
      worker = workers_[wid];
    }

    std::lock_guard<std::mutex> lock(request_lock_);
    for (auto it = request_queue_.begin(); it != request_queue_.end();) {
      auto req = *it;
      const auto maybe_satisfying = req->sel->is_satisfying(
          req->task_type, req->sector.proof_type, worker);
      if (maybe_satisfying.has_error()) {
        logger_->error("free worker satisfactory check: "
                       + maybe_satisfying.error().message());
        ++it;
        continue;
      }

      if (!maybe_satisfying.value()) {
        ++it;
        continue;
      }

      if (!worker->preparing.canHandleRequest(req->need_resources,
                                              worker->info.resources)) {
        ++it;
        continue;
      }

      assignWorker(wid, worker, req);

      it = request_queue_.erase(it);
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

  outcome::result<void> SchedulerImpl::resetWorks() {
    if (auto it{call_kv_->cursor()}) {
      boost::optional<WorkId> remove_id = boost::none;  // to not broke iterator
      for (it->seekToFirst(); it->isValid(); it->next()) {
        if (remove_id.has_value()) {
          OUTCOME_TRY(call_kv_->remove(static_cast<Bytes>(remove_id.value())));
          remove_id = boost::none;
        }
        OUTCOME_TRY(state, codec::cbor::decode<WorkState>(it->value()));
        switch (state.status) {
          case WorkStatus::kInProgress:
            break;
          default:
            remove_id = state.id;
        }
      }
      if (remove_id.has_value()) {
        OUTCOME_TRY(call_kv_->remove(static_cast<Bytes>(remove_id.value())));
      }
    }
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
