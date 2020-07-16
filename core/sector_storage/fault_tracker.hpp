/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_FAULT_TRACKER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_FAULT_TRACKER_HPP

#include <vector>
#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"

using fc::primitives::sector::RegisteredProof;
using fc::primitives::sector::SectorId;

namespace fc::sector_storage {

  class FaultTracker {
   public:
    virtual ~FaultTracker() = default;

    virtual outcome::result<std::vector<SectorId>> checkProvable(
        RegisteredProof seal_proof_type, gsl::span<const SectorId> sectors) = 0;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_FAULT_TRACKER_HPP
