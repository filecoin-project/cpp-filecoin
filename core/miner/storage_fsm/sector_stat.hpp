/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SECTOR_STAT_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SECTOR_STAT_HPP

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

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SECTOR_STAT_HPP
