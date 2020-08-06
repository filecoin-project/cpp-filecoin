/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sealing_impl.hpp"

namespace fc::mining {
  std::vector<SealingTransition> SealingImpl::makeFSMTransitions() {
    return {
        // Main pipeline
        SealingTransition(SealingEvent::kIncoming)
            .from(SealingState::kStateUnknown)
            .to(SealingState::kPacking),
        SealingTransition(SealingEvent::kPreCommit1)
            .fromMany(SealingState::kPacking,
                      SealingState::kSealPreCommit1Fail,
                      SealingState::kSealPreCommit2Fail)
            .to(SealingState::kPreCommit1),
        SealingTransition(SealingEvent::kPreCommit2)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kSealPreCommit2Fail)
            .to(SealingState::kPreCommit2),
        SealingTransition(SealingEvent::kPreCommit)
            .fromMany(SealingState::kPreCommit2, SealingState::kPreCommitFail)
            .to(SealingState::kPreCommitting),
        SealingTransition(SealingEvent::kWaitSeed)
            .fromMany(SealingState::kPreCommitting, SealingState::kCommitFail)
            .to(SealingState::kWaitSeed),
        SealingTransition(SealingEvent::kCommit)
            .fromMany(SealingState::kWaitSeed,
                      SealingState::kCommitFail,
                      SealingState::kComputeProofFail)
            .to(SealingState::kCommitting),
        SealingTransition(SealingEvent::kCommitWait)
            .from(SealingState::kCommitting)
            .to(SealingState::kCommitWait),
        SealingTransition(SealingEvent::kFinalizeSector)
            .fromMany(SealingState::kCommitWait, SealingState::kFinalizeFail)
            .to(SealingState::kFinalizeSector),
        SealingTransition(SealingEvent::kProve)
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kProving),

        // Errors
        SealingTransition(SealingEvent::kUnrecoverableFailed)
            .fromMany(SealingState::kPacking,
                      SealingState::kPreCommit1,
                      SealingState::kPreCommit2,
                      SealingState::kWaitSeed,
                      SealingState::kCommitting,
                      SealingState::kCommitWait,
                      SealingState::kProving)
            .to(SealingState::kFailUnrecoverable),
        SealingTransition(SealingEvent::kSealPreCommit1Failed)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kPreCommitting,
                      SealingState::kCommitFail)
            .to(SealingState::kSealPreCommit1Fail),
        SealingTransition(SealingEvent::kSealPreCommit2Failed)
            .from(SealingState::kPreCommit2)
            .to(SealingState::kSealPreCommit2Fail),
        SealingTransition(SealingEvent::kPreCommitFailed)
            .fromMany(SealingState::kPreCommitting, SealingState::kWaitSeed)
            .to(SealingState::kPreCommitFail),
        SealingTransition(SealingEvent::kComputeProofFailed)
            .from(SealingState::kCommitting)
            .to(SealingState::kComputeProofFail),
        SealingTransition(SealingEvent::kCommitFailed)
            .fromMany(SealingState::kCommitting, SealingState::kCommitWait)
            .to(SealingState::kCommitFail),
        SealingTransition(SealingEvent::kFinalizeFailed)
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kFinalizeFail),
    };
  }

}  // namespace fc::mining
