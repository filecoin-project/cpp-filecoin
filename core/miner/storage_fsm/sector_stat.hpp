/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/sealing_states.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::mining {
  using primitives::sector::SectorId;

  class SectorStat {
   public:
    virtual ~SectorStat() = default;

    virtual void updateSector(SectorId sector, SealingState state) = 0;

    virtual uint64_t currentSealing() const = 0;
  };
}  // namespace fc::mining
