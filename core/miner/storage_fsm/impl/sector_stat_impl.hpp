/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SECTOR_STAT_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SECTOR_STAT_IMPL_HPP

#include "miner/storage_fsm/sector_stat.hpp"

#include <map>
#include <mutex>

namespace fc::mining {
  enum StatState : uint64_t {
    kSealing = 0,
    kFailed,
    kProving,
    kAmount,  // END
  };

  class SectorStatImpl : public SectorStat {
   public:
    SectorStatImpl();

    void updateSector(SectorId sector, SealingState state) override;

    uint64_t currentSealing() const override;

   private:
    mutable std::mutex mutex_;

    std::map<SectorId, uint64_t> by_sector_;
    std::vector<uint64_t> totals_;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SECTOR_STAT_IMPL_HPP
