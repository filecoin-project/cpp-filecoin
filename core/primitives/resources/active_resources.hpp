/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_ACTIVE_RESOURCES_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_ACTIVE_RESOURCES_HPP

#include "primitives/resources/resources.hpp"

namespace fc::primitives {

  struct ActiveResources {
    uint64_t memory_used_min;
    uint64_t memory_used_max;
    bool gpu_used;
    uint64_t cpu_use;
  };

  bool canHandleRequest(const Resources &need_resources,
                        const WorkerResources &resources,
                        const ActiveResources &active);

}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_SEAL_RESOURCES_ACTIVE_RESOURCES_HPP
