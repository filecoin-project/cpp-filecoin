/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/selector.hpp"

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage {
  class ExistingSelector : public WorkerSelector {
   public:
    ExistingSelector(std::shared_ptr<stores::SectorIndex> index,
                     SectorId sector,
                     SectorFileType allocate,
                     bool allow_fetch);

    outcome::result<bool> is_satisfying(
        const TaskType &task,
        RegisteredSealProof seal_proof_type,
        const std::shared_ptr<WorkerHandle> &worker) override;

    outcome::result<bool> is_preferred(
        const TaskType &task,
        const std::shared_ptr<WorkerHandle> &challenger,
        const std::shared_ptr<WorkerHandle> &current_best) override;

   private:
    std::shared_ptr<stores::SectorIndex> index_;
    SectorId sector_;
    SectorFileType allocate_;
    bool allow_fetch_;
  };
}  // namespace fc::sector_storage
