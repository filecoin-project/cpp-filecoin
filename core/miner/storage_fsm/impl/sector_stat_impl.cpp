/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sector_stat_impl.hpp"

namespace fc::mining {

  StatState toStatState(SealingState state) {
    switch (state) {
      case SealingState::kWaitDeals:
      case SealingState::kPacking:
      case SealingState::kPreCommit1:
      case SealingState::kPreCommit2:
      case SealingState::kPreCommitting:
      case SealingState::kPreCommittingWait:
      case SealingState::kWaitSeed:
      case SealingState::kCommitting:
      case SealingState::kCommitWait:
      case SealingState::kFinalizeSector:
      case SealingState::kSnapDealsPacking:
      case SealingState::kUpdateReplica:
      case SealingState::kProveReplicaUpdate:
      case SealingState::kFinalizeReplicaUpdate:
        return StatState::kSealing;
      case SealingState::kProving:
      case SealingState::kRemoved:
      case SealingState::kRemoving:
      case SealingState::kUpdateActivating:
      case SealingState::kReleaseSectorKey:
        return StatState::kProving;
      default:
        return StatState::kFailed;
    }
  }

  SectorStatImpl::SectorStatImpl() {
    totals_.resize(StatState::kAmount, 0);
  }

  void SectorStatImpl::updateSector(SectorId sector, SealingState state) {
    std::lock_guard lock(mutex_);

    auto maybe_sector = by_sector_.find(sector);
    if (maybe_sector != by_sector_.end()) {
      totals_[maybe_sector->second]--;
    }

    uint64_t stat_state = toStatState(state);
    by_sector_[sector] = stat_state;
    totals_[stat_state]++;
  }

  uint64_t SectorStatImpl::currentSealing() const {
    std::lock_guard lock(mutex_);

    return totals_[StatState::kSealing] + totals_[StatState::kFailed];
  }
}  // namespace fc::mining
