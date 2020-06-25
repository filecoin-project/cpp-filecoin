/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_WORKER_CONFIG_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_WORKER_CONFIG_HPP

#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::primitives {
  struct WorkerConfig {
    std::string hostname;
    sector::RegisteredProof seal_proof_type;
    std::vector<TaskType> task_types;
  };
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_WORKER_CONFIG_HPP
