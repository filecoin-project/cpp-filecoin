/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_TASK_SELECTOR_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_TASK_SELECTOR_HPP

#include "sector_storage/selector.hpp"

namespace fc::sector_storage {

  class TaskSelector : public WorkerSelector {
  public:
      outcome::result<bool> is_satisfying(const TaskType &task, RegisteredProof seal_proof_type,
                                          const std::shared_ptr<WorkerHandle> &worker) override;

      outcome::result<bool> is_preferred(const TaskType &task, const std::shared_ptr<WorkerHandle> &challenger,
                                         const std::shared_ptr<WorkerHandle> &current_best) override;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_TASK_SELECTOR_HPP
