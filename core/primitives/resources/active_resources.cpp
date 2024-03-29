/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/resources/active_resources.hpp"

namespace fc::primitives {
  bool ActiveResources::canHandleRequest(const Resources &need_resources,
                                         const WorkerResources &resources) {
    std::shared_lock lock(mutex_);

    // TODO (ortyomka): dedupe need_resources.base_min_memory per task type
    // (don't add if that task is already running)
    uint64_t min_need_memory = resources.reserved_memory + memory_used_min
                               + need_resources.min_memory
                               + need_resources.base_min_memory;
    if (min_need_memory > resources.physical_memory) {
      return false;
    }

    uint64_t max_need_memory = resources.reserved_memory + memory_used_min
                               + need_resources.max_memory
                               + need_resources.base_min_memory;
    if (max_need_memory > (resources.swap_memory + resources.physical_memory)) {
      return false;
    }

    if (need_resources.threads) {
      if ((cpu_use + *need_resources.threads) > resources.cpus) {
        return false;
      }
    } else {
      if (cpu_use > 0) {
        return false;
      }
    }

    if (!resources.gpus.empty() && need_resources.can_gpu) {
      if (gpu_used) {
        return false;
      }
    }

    return true;
  }

  void ActiveResources::add(const WorkerResources &worker_resources,
                            const Resources &resources) {
    std::unique_lock lock(mutex_);
    if (resources.can_gpu) {
      gpu_used = true;
    }
    if (resources.threads) {
      cpu_use += *resources.threads;
    } else {
      cpu_use += worker_resources.cpus;
    }

    memory_used_min += resources.min_memory;
    memory_used_max += resources.max_memory;
  }

  void ActiveResources::free(const WorkerResources &worker_resources,
                             const Resources &resources) {
    std::unique_lock lock(mutex_);
    if (resources.can_gpu) {
      gpu_used = false;
    }

    if (resources.threads) {
      cpu_use -= *resources.threads;
    } else {
      cpu_use -= worker_resources.cpus;
    }

    memory_used_min -= resources.min_memory;
    memory_used_max -= resources.max_memory;
  }

  double ActiveResources::utilization(const WorkerResources &worker_resources) {
    std::shared_lock lock(mutex_);
    double max = static_cast<double>(cpu_use) / worker_resources.cpus;

    double memory_min =
        static_cast<double>(memory_used_min + worker_resources.reserved_memory)
        / worker_resources.physical_memory;
    if (memory_min > max) {
      max = memory_min;
    }

    double memory_max =
        static_cast<double>(memory_used_max + worker_resources.reserved_memory)
        / static_cast<double>(worker_resources.physical_memory
                              + worker_resources.swap_memory);
    if (memory_max > max) {
      max = memory_max;
    }

    return max;
  }
}  // namespace fc::primitives
