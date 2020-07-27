/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_ALLOCATE_SELECTOR_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_ALLOCATE_SELECTOR_HPP

#include "sector_storage/selector.hpp"

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage {
  class AllocateSelector : public WorkerSelector {
   public:
    AllocateSelector(std::shared_ptr<stores::SectorIndex> index,
                     SectorFileType allocate,
                     bool sealing);

    outcome::result<bool> is_satisfying(
        const TaskType &task,
        RegisteredProof seal_proof_type,
        const std::shared_ptr<WorkerHandle> &worker) override;

    outcome::result<bool> is_preferred(
        const TaskType &task,
        const std::shared_ptr<WorkerHandle> &challenger,
        const std::shared_ptr<WorkerHandle> &current_best) override;

   private:
    std::shared_ptr<stores::SectorIndex> sector_index_;
    SectorFileType allocate_;
    bool sealing_;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_ALLOCATE_SELECTOR_HPP
