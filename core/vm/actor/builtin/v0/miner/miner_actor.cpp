/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::actor::builtin::v0::miner {
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

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account or multisig actor
   * @param address to resolve
   * @return resolved address
   */
  outcome::result<Address> resolveControlAddress(Runtime &runtime,
                                                 const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    VM_ASSERT(resolved.isId());
    const auto resolved_code = runtime.getActorCodeID(resolved);
    OUTCOME_TRY(
        runtime.requireNoError(resolved_code, VMExitCode::kErrIllegalArgument));
    OUTCOME_TRY(
        runtime.validateArgument(isSignableActorV0(resolved_code.value())));
    return std::move(resolved);
  }

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account actor with an associated BLS key. The worker must be BLS since the
   * worker key will be used alongside a BLS-VRF.
   * @param runtime
   * @param address to resolve
   * @return resolved address
   */
  outcome::result<Address> resolveWorkerAddress(Runtime &runtime,
                                                const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    VM_ASSERT(resolved.isId());
    const auto resolved_code = runtime.getActorCodeID(resolved);
    OUTCOME_TRY(
        runtime.requireNoError(resolved_code, VMExitCode::kErrIllegalArgument));
    OUTCOME_TRY(
        runtime.validateArgument(resolved_code.value() == kAccountCodeCid));

    if (!address.isBls()) {
      OUTCOME_TRY(pubkey_addres,
                  runtime.sendM<account::PubkeyAddress>(resolved, {}, 0));
      OUTCOME_TRY(runtime.validateArgument(pubkey_addres.isBls()));
    }

    return std::move(resolved);
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    // proof is supported
    OUTCOME_TRY(
        runtime.validateArgument(kSupportedProofs.find(params.seal_proof_type)
                                 != kSupportedProofs.end()));
    OUTCOME_TRY(owner, resolveControlAddress(runtime, params.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.worker));
    //    worker := resolveWorkerAddress(rt, params.WorkerAddr)

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

  ACTOR_METHOD_IMPL(ConfirmSectorProofsValid) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeMultiaddresses) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CompactPartitions) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CompactSectorNumbers) {
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
      exportMethod<ConfirmSectorProofsValid>(),
      exportMethod<ChangeMultiaddresses>(),
      exportMethod<CompactPartitions>(),
      exportMethod<CompactSectorNumbers>(),
  };
}  // namespace fc::vm::actor::builtin::v0::miner
