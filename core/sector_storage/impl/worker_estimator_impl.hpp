/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/worker_estimator.hpp"

#include <chrono>
#include <shared_mutex>

#include "sector_storage/worker.hpp"

namespace fc::sector_storage {

  class EstimatorImpl : public Estimator {
   public:
    EstimatorImpl(uint64_t count_calls);

    void startWork(WorkerId worker_id, TaskType type, CallId call_id) override;

    void finishWork(CallId call_id) override;

    void abortWork(CallId call_id) override;

    boost::optional<double> getTime(WorkerId id, TaskType type) const override;

   private:
    mutable std::shared_mutex mutex_;

    struct ActiveWork {
      TaskType type;
      WorkerId worker{};
      std::chrono::steady_clock::time_point start;
    };

    std::map<CallId, ActiveWork> active_works_;

    uint64_t count_calls_;

    class CallsData {
     public:
      CallsData(uint64_t count_calls)
          : count_calls_(count_calls), dirty_flag_(false), average_(0){};

      inline void addData(uint64_t enrty) {
        dirty_flag_ = true;
        data_.push_back(enrty);

        if (data_.size() > count_calls_) {
          data_.pop_front();
        }
      }

      boost::optional<double> getAverage() const {
        if (data_.empty()) {
          return boost::none;
        }

        if (dirty_flag_) {
          auto size = data_.size();

          uint64_t sum = 0;
          for (size_t i = 0; i < size; i++) {
            sum += data_.at(i) * (size - i);
          }

          // note: weighted moving average
          average_ = static_cast<double>(sum) / double(size * size + 1) * 2;
          dirty_flag_ = false;
        }

        return average_;
      }

     private:
      std::deque<uint64_t> data_;
      uint64_t count_calls_;

      mutable bool dirty_flag_ = false;
      mutable double average_;
    };

    std::map<WorkerId, std::map<TaskType, CallsData>> workers_data_;
  };

}  // namespace fc::sector_storage
