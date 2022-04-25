/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/worker_estimator_impl.hpp"

namespace fc::sector_storage {

  EstimatorImpl::EstimatorImpl(uint64_t window_size)
      : window_size_(window_size) {}

  void EstimatorImpl::startWork(WorkerId worker_id,
                                TaskType type,
                                CallId call_id) {
    const auto start = std::chrono::steady_clock::now();
    std::lock_guard locker(mutex_);
    active_works_[call_id] = ActiveWork{type, worker_id, start};
  }

  void EstimatorImpl::finishWork(CallId call_id) {
    const auto finish = std::chrono::steady_clock::now();

    std::lock_guard locker(mutex_);
    const auto it = active_works_.find(call_id);
    if (it == active_works_.end()) return;
    const auto &work{it->second};

    auto &task_map{workers_data_[work.worker]};
    auto it2 = task_map.find(work.type);
    if (it2 == task_map.end()) {
      std::tie(it2, std::ignore) =
          task_map.try_emplace(work.type, CallsData(window_size_));
    }
    it2->second.addData(std::chrono::duration_cast<std::chrono::milliseconds>(
                            finish - work.start)
                            .count());

    active_works_.erase(it);
  }

  void EstimatorImpl::abortWork(CallId call_id) {
    std::lock_guard locker(mutex_);
    const auto it = active_works_.find(call_id);
    if (it == active_works_.end()) return;

    // we also can calculate how much works were aborted. It can be not worker's
    // guilt, but if we want to have this statistic we can.

    active_works_.erase(it);
  }

  boost::optional<double> EstimatorImpl::getTime(WorkerId id,
                                                 TaskType type) const {
    std::shared_lock locker(mutex_);

    const auto tasks_it = workers_data_.find(id);
    if (tasks_it == workers_data_.end()) {
      return boost::none;
    }

    const auto it = tasks_it->second.find(type);
    if (it == tasks_it->second.end()) {
      return boost::none;
    }

    return it->second.getAverage();
  }
}  // namespace fc::sector_storage
