/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_STATES_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_STATES_HPP

#include <cstdint>

namespace fc::mining {
  /**
   * SealingState is an state that occurs in a sealing lifecycle
   */
  enum class SealingState : uint64_t {
    kStateUnknown = 0,
    kSealPreCommit1Fail,
    kSealPreCommit2Fail,
    kPreCommitFail,
    kComputeProofFail,
    kCommitFail,
    kFinalizeFail,

    // Internal
    kPacking,
    kWaitDeals,
    kPreCommit1,
    kPreCommit2,
    kPreCommitting,
    kPreCommittingWait,
    kWaitSeed,
    kCommitting,
    kCommitWait,
    kFinalizeSector,
    kProving,

    // Post states
    kFaulty,
    kFaultReported,

    kRemoving,
    kRemoveFail,
    kRemoved,
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_STATES_HPP
