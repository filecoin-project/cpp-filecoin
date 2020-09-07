/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/miner/miner_actor.hpp"


namespace fc::vm::actor::builtin::miner {
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
