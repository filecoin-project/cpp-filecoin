/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_ACTIVE_RESOURCES_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_ACTIVE_RESOURCES_HPP

#include <shared_mutex>
#include "primitives/resources/resources.hpp"
#include "primitives/types.hpp"

namespace fc::primitives {

  struct ActiveResources {
    uint64_t memory_used_min = 0;
    uint64_t memory_used_max = 0;
    bool gpu_used = false;
    uint64_t cpu_use = 0;

    void add(const WorkerResources &worker_resources,
             const Resources &resources);

    void free(const WorkerResources &worker_resources,
              const Resources &resources);

    /**
     * @brief run @callback with @resources
     */
    outcome::result<void> withResources(
        const WorkerResources &worker_resources,
        const Resources &resources,
        std::mutex &locker,
        const std::function<outcome::result<void>()> &callback);

    double utilization(const WorkerResources &worker_resources);

    friend bool canHandleRequest(const Resources &need_resources,
                                 const WorkerResources &resources,
                                 const ActiveResources &active);

   private:
    mutable std::shared_mutex mutex_;
    bool unlock_;
    std::condition_variable cv_;
  };

  bool canHandleRequest(const Resources &need_resources,
                        const WorkerResources &resources,
                        const ActiveResources &active);

}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_SEAL_RESOURCES_ACTIVE_RESOURCES_HPP
