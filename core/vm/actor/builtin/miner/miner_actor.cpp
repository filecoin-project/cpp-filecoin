/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::miner {
  DeadlineInfo DeadlineInfo::make(ChainEpoch start,
                                  size_t index,
                                  ChainEpoch now) {
    if (index < kWPoStPeriodDeadlines) {
      auto open{start + static_cast<ChainEpoch>(index) * kWPoStChallengeWindow};
      return {now,
              start,
              index,
              open,
              open + kWPoStChallengeWindow,
              open - kWPoStChallengeLookback,
              open - kFaultDeclarationCutoff};
    }
    auto after{start + kWPoStProvingPeriod};
    return {now, start, index, after, after, after, 0};
  }

  DeadlineInfo State::deadlineInfo(ChainEpoch now) const {
    auto progress{now - proving_period_start};
    size_t deadline{kWPoStPeriodDeadlines};
    if (progress < kWPoStProvingPeriod) {
      deadline = std::max(ChainEpoch{0}, progress / kWPoStChallengeWindow);
    }
    return DeadlineInfo::make(proving_period_start, deadline, now);
  }

  ACTOR_METHOD_IMPL(Construct) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ControlAddresses) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangePeerId) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(SubmitWindowedPoSt) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(PreCommitSector) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ProveCommitSector) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ExtendSectorExpiration) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(TerminateSectors) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(DeclareFaults) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(DeclareFaultsRecovered) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(OnDeferredCronEvent) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CheckSectorProven) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(AddLockedFund) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ReportConsensusFault) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    return VMExitCode::kNotImplemented;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<ControlAddresses>(),
      exportMethod<ChangeWorkerAddress>(),
      exportMethod<ChangePeerId>(),
      exportMethod<SubmitWindowedPoSt>(),
      exportMethod<PreCommitSector>(),
      exportMethod<ProveCommitSector>(),
      exportMethod<ExtendSectorExpiration>(),
      exportMethod<TerminateSectors>(),
      exportMethod<DeclareFaults>(),
      exportMethod<DeclareFaultsRecovered>(),
      exportMethod<OnDeferredCronEvent>(),
      exportMethod<CheckSectorProven>(),
      exportMethod<AddLockedFund>(),
      exportMethod<ReportConsensusFault>(),
      exportMethod<WithdrawBalance>(),
  };
}  // namespace fc::vm::actor::builtin::miner
