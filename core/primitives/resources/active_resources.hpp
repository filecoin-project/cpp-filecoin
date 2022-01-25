/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <condition_variable>
#include <shared_mutex>
#include "primitives/resources/resources.hpp"
#include "primitives/types.hpp"

namespace fc::primitives {

  struct ActiveResources {
    void add(const WorkerResources &worker_resources,
             const Resources &resources);

    void free(const WorkerResources &worker_resources,
              const Resources &resources);

    double utilization(const WorkerResources &worker_resources);

    inline void setMemoryUsedMin(uint64_t memory) {
      std::unique_lock lock{mutex_};
      memory_used_min = memory;
    }

    bool canHandleRequest(const Resources &need_resources,
                          const WorkerResources &resources);

   private:
    uint64_t memory_used_min = 0;
    uint64_t memory_used_max = 0;
    bool gpu_used = false;
    uint64_t cpu_use = 0;
    mutable std::shared_mutex mutex_;
  };

}  // namespace fc::primitives
