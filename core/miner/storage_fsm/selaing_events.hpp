/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP

#include <cstdint>

namespace fc::mining {
  /**
   * SealingEvent is an event that occurs in a sealing lifecycle
   */
  enum class SealingEvent : uint64_t {
    kIncoming = 1,
    kPreCommit1,
    kPreCommit2,
    kPreCommit,
    kPreCommitWait,
    kWaitSeed,
    kCommit,
    kCommitWait,
    kFinalizeSector,
    kProve,

    kFinalizeFailed,
    kCommitFailed,
    kComputeProofFailed,
    kPreCommitFailed,
    kSealPreCommit2Failed,
    kSealPreCommit1Failed,
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP
