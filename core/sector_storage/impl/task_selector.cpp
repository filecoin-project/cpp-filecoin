/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/task_selector.hpp"

libp2p::outcome::result<bool> fc::sector_storage::TaskSelector::is_satisfying(
    const fc::primitives::TaskType &task,
    RegisteredProof seal_proof_type,
    const std::shared_ptr<WorkerHandle> &worker) {
  OUTCOME_TRY(tasks, worker->worker->getSupportedTask());
  return tasks.find(task) != tasks.end();
}

libp2p::outcome::result<bool> fc::sector_storage::TaskSelector::is_preferred(
    const fc::primitives::TaskType &task,
    const std::shared_ptr<WorkerHandle> &challenger,
    const std::shared_ptr<WorkerHandle> &current_best) {
  OUTCOME_TRY(challenger_tasks, challenger->worker->getSupportedTask());
  OUTCOME_TRY(current_best_tasks, current_best->worker->getSupportedTask());

  if (challenger_tasks.size() != current_best_tasks.size()) {
    return challenger_tasks.size() < current_best_tasks.size();
  }

  return challenger->active.utilization(challenger->info.resources)
         < current_best->active.utilization(current_best->info.resources);
}
