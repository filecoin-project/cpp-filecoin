/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_EXISTING_SELECTOR_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_EXISTING_SELECTOR_HPP

#include "sector_storage/selector.hpp"

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage {
  class ExistingSelector : public WorkerSelector {
   public:
    static outcome::result<std::unique_ptr<WorkerSelector>> newExistingSelector(
        const std::shared_ptr<stores::SectorIndex> &index,
        const SectorId &sector,
        SectorFileType allocate,
        bool allow_fetch);

    outcome::result<bool> is_satisfying(
        const TaskType &task,
        RegisteredProof seal_proof_type,
        const std::shared_ptr<WorkerHandle> &worker) override;

    outcome::result<bool> is_preferred(
        const TaskType &task,
        const std::shared_ptr<WorkerHandle> &challenger,
        const std::shared_ptr<WorkerHandle> &current_best) override;

   private:
    explicit ExistingSelector(std::set<primitives::StorageID> best);

    std::set<primitives::StorageID> best_;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_EXISTING_SELECTOR_HPP
