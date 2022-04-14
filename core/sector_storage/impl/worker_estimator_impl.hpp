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
    EstimatorImpl(uint64_t window_size);

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

    const uint64_t window_size_;

    class CallsData {
     public:
      CallsData(uint64_t window_size) : window_size_(window_size){};

      inline void addData(uint64_t enrty) {
        average_ = boost::none;
        data_.push_back(enrty);

        if (data_.size() > window_size_) {
          data_.pop_front();
        }
      }

      boost::optional<double> getAverage() const {
        if (data_.empty()) {
          return boost::none;
        }

        if (not average_.has_value()) {
          auto size = data_.size();

          uint64_t sum = 0;
          size_t weight{1};
          for (auto value : data_) {
            sum += value * weight;
            ++weight;
          }

          // note: weighted moving average
          average_ = static_cast<double>(sum) / double(size * (size + 1)) * 2;
        }

        return average_;
      }

     private:
      std::deque<uint64_t> data_;
      const uint64_t window_size_;

      mutable boost::optional<double> average_;
    };

    std::map<WorkerId, std::map<TaskType, CallsData>> workers_data_;
  };

}  // namespace fc::sector_storage
