/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/resources/active_resources.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "sector_storage/worker.hpp"

namespace fc::sector_storage {
  using primitives::ActiveResources;
  using primitives::TaskType;
  using primitives::sector::RegisteredSealProof;

  struct WorkerHandle {
    std::shared_ptr<Worker> worker;
    primitives::WorkerInfo info;

    ActiveResources preparing;
    ActiveResources active;
  };

  inline bool operator==(const WorkerHandle &lhs, const WorkerHandle &rhs) {
    return lhs.info.hostname == rhs.info.hostname
           && lhs.info.resources == rhs.info.resources;
  }

  class WorkerSelector {
   public:
    virtual ~WorkerSelector() = default;

    virtual outcome::result<bool> is_satisfying(
        const TaskType &task,
        RegisteredSealProof seal_proof_type,
        const std::shared_ptr<WorkerHandle> &worker) = 0;

    virtual outcome::result<bool> is_preferred(
        const TaskType &task,
        const std::shared_ptr<WorkerHandle> &challenger,
        const std::shared_ptr<WorkerHandle> &current_best) = 0;
  };
}  // namespace fc::sector_storage
