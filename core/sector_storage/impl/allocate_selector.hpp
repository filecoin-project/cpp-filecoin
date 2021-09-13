/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/selector.hpp"

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage {
  class AllocateSelector : public WorkerSelector {
   public:
    AllocateSelector(std::shared_ptr<stores::SectorIndex> index,
                     SectorFileType allocate,
                     PathType path_type);

    outcome::result<bool> is_satisfying(
        const TaskType &task,
        RegisteredSealProof seal_proof_type,
        const std::shared_ptr<WorkerHandle> &worker) override;

    outcome::result<bool> is_preferred(
        const TaskType &task,
        const std::shared_ptr<WorkerHandle> &challenger,
        const std::shared_ptr<WorkerHandle> &current_best) override;

   private:
    std::shared_ptr<stores::SectorIndex> sector_index_;
    SectorFileType allocate_;
    PathType path_type_;
  };
}  // namespace fc::sector_storage
