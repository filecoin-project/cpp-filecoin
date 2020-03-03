/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/syncer_state.hpp"

namespace fc::blockchain {

  std::string toString(SyncStateStage value) {
    switch (value) {
      case SyncStateStage::StageIdle:
        return "idle";
      case SyncStateStage::StageHeaders:
        return "header sync";
      case SyncStateStage::StagePersistHeaders:
        return "persisting headers";
      case SyncStateStage::StageMessages:
        return "message sync";
      case SyncStateStage::StageSyncComplete:
        return "complete";
      case SyncStateStage::StageSyncErrored:
        return "error";
      default:
        return std::string("unknown stage: "
                           + std::to_string(static_cast<int>(value)));
    }
  }

  void SyncerState::initialize(Tipset b, Tipset t) {
    target = std::move(b);
    base = std::move(t);

    clock::UTCClockImpl clock{};
    start = clock.nowUTC();
    end = clock.nowUTC();
  }

  void SyncerState::setStage(SyncStateStage s) {
    stage = s;

    if (stage == SyncStateStage::StageSyncComplete) {
      end = clock::UTCClockImpl{}.nowUTC();
    }
  }

  void SyncerState::setError(outcome::result<void> e) {
    error = e;
    stage = SyncStateStage::StageSyncErrored;
    end = clock::UTCClockImpl{}.nowUTC();
  }

  SyncerState SyncerState::takeSnapshot() const {
    return *this;
  }

}  // namespace fc::blockchain
