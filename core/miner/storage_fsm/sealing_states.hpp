/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
    kDealsExpired,
    kRecoverDealIDs,

    // Internal
    kPacking,
    kWaitDeals,
    kPreCommit1,
    kPreCommit2,
    kPreCommitting,
    kSubmitPreCommitBatch,
    kPreCommittingWait,
    kWaitSeed,
    kComputeProof,
    kCommitting,
    kCommitWait,
    kFinalizeSector,
    kProving,

    // snap deals / cc update
    kSnapDealsWaitDeals,
    kSnapDealsAddPiece,
    kSnapDealsPacking,
    kUpdateReplica,
    kProveReplicaUpdate,
    kSubmitReplicaUpdate,
    kReplicaUpdateWait,
    kFinalizeReplicaUpdate,
    kUpdateActivating,
    kReleaseSectorKey,

    // snap deals error modes
    kSnapDealsAddPieceFailed,
    kSnapDealsDealsExpired,
    kSnapDealsRecoverDealIDs,
    kAbortUpgrade,
    kReplicaUpdateFailed,
    kReleaseSectorKeyFailed,
    kFinalizeReplicaUpdateFailed,

    // Post states
    kFaulty,
    kFaultReported,

    kRemoving,
    kRemoveFail,
    kRemoved,

    // Hacks
    kForce,
  };
}  // namespace fc::mining
