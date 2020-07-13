/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_SELECTOR_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_SELECTOR_HPP

#include "common/outcome.hpp"
#include "primitives/resources/active_resources.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "sector_storage/worker.hpp"

namespace fc::sector_storage {
  using primitives::ActiveResources;
  using primitives::TaskType;
  using primitives::sector::RegisteredProof;

  struct WorkerHandle {
    std::shared_ptr<Worker> worker;
    primitives::WorkerInfo info;

    ActiveResources preparing;
    ActiveResources active;
  };

  class WorkerSelector {
   public:
    virtual ~WorkerSelector() = default;

    virtual outcome::result<bool> is_satisfying(
        const TaskType &task,
        RegisteredProof seal_proof_type,
        const std::shared_ptr<WorkerHandle> &worker) = 0;

    virtual outcome::result<bool> is_preferred(
        const TaskType &task,
        const std::shared_ptr<WorkerHandle> &challenger,
        const std::shared_ptr<WorkerHandle> &current_best) = 0;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_SELECTOR_HPP
