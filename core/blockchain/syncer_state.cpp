/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/syncer_state.hpp"

namespace fc::blockchain {

  std::string toString(SyncStateStage value) {
    switch (value) {
      case SyncStateStage::STAGE_IDLE:
        return "idle";
      case SyncStateStage::STAGE_HEADERS:
        return "header sync";
      case SyncStateStage::STAGE_PERSIST_HEADERS:
        return "persisting headers";
      case SyncStateStage::STAGE_MESSAGES:
        return "message sync";
      case SyncStateStage::STAGE_SYNC_COMPLETE:
        return "complete";
      case SyncStateStage::STAGE_SYNC_ERRORED:
        return "error";
      default:
        return std::string("unknown stage: "
                           + std::to_string(static_cast<int>(value)));
    }
  }

  void SyncerState::initialize(Tipset b, Tipset t) {
    target = std::move(b);
    base = std::move(t);

    clock::UTCClockImpl clock;
    start = clock.nowUTC();
    end = clock.nowUTC();
  }

  void SyncerState::setStage(SyncStateStage s) {
    stage = s;

    if (SyncStateStage::STAGE_SYNC_COMPLETE == stage) {
      end = clock::UTCClockImpl{}.nowUTC();
    }
  }

  void SyncerState::setError(outcome::result<void> e) {
    error = e;
    stage = SyncStateStage::STAGE_SYNC_ERRORED;
    end = clock::UTCClockImpl{}.nowUTC();
  }

  SyncerState SyncerState::takeSnapshot() const {
    return *this;
  }

}  // namespace fc::blockchain
