/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/resources/active_resources.hpp"

namespace fc::primitives {
    bool canHandleRequest(const Resources &need_resources,
                          const WorkerResources &resources,
                          const ActiveResources &active) {
        // TODO: dedupe need_resources.base_min_memory per task type
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
}
