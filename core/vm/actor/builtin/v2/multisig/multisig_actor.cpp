/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/multisig/multisig_actor.hpp"

#include "vm/actor/builtin/states/multisig_actor_state.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::multisig {
  using common::Buffer;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using states::MultisigActorStatePtr;
  using toolchain::Toolchain;

  // Construct
  //============================================================================

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    OUTCOME_TRY(runtime.validateArgument(!params.signers.empty()));
    OUTCOME_TRY(runtime.validateArgument(params.signers.size() <= kSignersMax));
    OUTCOME_TRY(
        resolved_signers,
        v0::multisig::Construct::getResolvedSigners(runtime, params.signers));
    OUTCOME_TRY(v0::multisig::Construct::checkParams(
        params.signers, params.threshold, params.unlock_duration));
    auto state = v0::multisig::Construct::createState(
        runtime, params.threshold, resolved_signers);

    if (params.unlock_duration != 0) {
      state->setLocked(params.start_epoch,
                       params.unlock_duration,
                       runtime.getValueReceived());
    }

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // AddSigner
  //============================================================================

  outcome::result<void> AddSigner::checkSignersCount(
      const std::vector<Address> &signers) {
    if (signers.size() >= kSignersMax) {
      ABORT(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(resolved_signer, utils->getResolvedAddress(params.signer));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(checkSignersCount(state->signers));
    OUTCOME_TRY(
        v0::multisig::AddSigner::addSigner(params, state, resolved_signer));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // RemoveSigner
  //============================================================================

  ACTOR_METHOD_IMPL(RemoveSigner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(resolved_signer, utils->getResolvedAddress(params.signer));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(
        v0::multisig::RemoveSigner::checkState(params, state, resolved_signer));

    if (params.decrease_threshold) {
      if (state->threshold < 2) {
        ABORT(VMExitCode::kErrIllegalState);
      }
      state->threshold--;
    }

    state->signers.erase(std::find(
        state->signers.begin(), state->signers.end(), resolved_signer));

    REQUIRE_NO_ERROR(utils->purgeApprovals(state, resolved_signer),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // SwapSigner
  //============================================================================

  ACTOR_METHOD_IMPL(SwapSigner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(from_resolved, utils->getResolvedAddress(params.from));
    OUTCOME_TRY(to_resolved, utils->getResolvedAddress(params.to));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(v0::multisig::SwapSigner::swapSigner(
        state, from_resolved, to_resolved));
    REQUIRE_NO_ERROR(utils->purgeApprovals(state, from_resolved),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // LockBalance
  //============================================================================

  ACTOR_METHOD_IMPL(LockBalance) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(runtime.validateArgument(params.unlock_duration > 0));
    OUTCOME_TRY(runtime.validateArgument(
        !((runtime.getNetworkVersion() >= NetworkVersion::kVersion7)
          && (params.amount < 0))));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(v0::multisig::LockBalance::lockBalance(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Propose>(),
      exportMethod<Approve>(),
      exportMethod<Cancel>(),
      exportMethod<AddSigner>(),
      exportMethod<RemoveSigner>(),
      exportMethod<SwapSigner>(),
      exportMethod<ChangeThreshold>(),
      exportMethod<LockBalance>(),
  };
}  // namespace fc::vm::actor::builtin::v2::multisig
