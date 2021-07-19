/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::sector_storage {
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::SectorId;
  using primitives::sector::SectorRef;

  class FaultTracker {
   public:
    virtual ~FaultTracker() = default;

    virtual outcome::result<std::vector<SectorId>> checkProvable(
        RegisteredPoStProof proof_type,
        gsl::span<const SectorRef> sectors) const = 0;
  };

}  // namespace fc::sector_storage
