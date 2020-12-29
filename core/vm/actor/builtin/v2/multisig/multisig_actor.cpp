/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v2/multisig/impl/multisig_utils_impl_v2.hpp"

namespace fc::vm::actor::builtin::v2::multisig {

  using fc::common::Buffer;
  using fc::primitives::BigInt;
  using fc::primitives::ChainEpoch;
  using fc::vm::VMExitCode;
  using fc::vm::actor::ActorExports;
  using fc::vm::actor::ActorMethod;
  using fc::vm::actor::kInitAddress;
  using fc::vm::runtime::InvocationOutput;
  using fc::vm::runtime::Runtime;
  namespace outcome = fc::outcome;

  // Construct
  //============================================================================

  outcome::result<void> Construct::checkSignersCount(
      const std::vector<Address> &signers) {
    if (signers.size() > kSignersMax) {
      return VMExitCode::kErrIllegalArgument;
    }
    return outcome::success();
  }

  void Construct::setLocked(const Runtime &runtime,
                            const ChainEpoch &start_epoch,
                            const EpochDuration &unlock_duration,
                            State &state) {
    if (unlock_duration != 0) {
      state.setLocked(start_epoch, unlock_duration, runtime.getValueReceived());
    }
  }

  outcome::result<Construct::Result> Construct::execute(
      Runtime &runtime,
      const Construct::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    OUTCOME_TRY(v0::multisig::Construct::checkEmptySigners(params.signers));
    OUTCOME_TRY(checkSignersCount(params.signers));
    OUTCOME_TRY(
        resolved_signers,
        v0::multisig::Construct::getResolvedSigners(runtime, params.signers));
    OUTCOME_TRY(v0::multisig::Construct::checkParams(
        params.signers, params.threshold, params.unlock_duration));
    State state = v0::multisig::Construct::createState(
        runtime, params.threshold, resolved_signers);
    setLocked(runtime, params.start_epoch, params.unlock_duration, state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // Propose
  //============================================================================

  outcome::result<Propose::Result> Propose::execute(
      Runtime &runtime,
      const Propose::Params &params,
      const MultisigUtils &utils) {
    return v0::multisig::Propose::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(Propose) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // Approve
  //============================================================================

  outcome::result<Approve::Result> Approve::execute(
      Runtime &runtime,
      const Approve::Params &params,
      const MultisigUtils &utils) {
    return v0::multisig::Approve::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(Approve) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // Cancel
  //============================================================================

  outcome::result<Cancel::Result> Cancel::execute(Runtime &runtime,
                                                  const Cancel::Params &params,
                                                  const MultisigUtils &utils) {
    return v0::multisig::Cancel::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(Cancel) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // AddSigner
  //============================================================================

  outcome::result<void> AddSigner::checkSignersCount(
      const std::vector<Address> &signers) {
    if (signers.size() >= kSignersMax) {
      return VMExitCode::kErrForbidden;
    }
    return outcome::success();
  }

  outcome::result<AddSigner::Result> AddSigner::execute(
      Runtime &runtime,
      const AddSigner::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(resolved_signer,
                utils.getResolvedAddress(runtime, params.signer));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(checkSignersCount(state.signers));
    OUTCOME_TRY(
        v0::multisig::AddSigner::addSigner(params, state, resolved_signer));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // RemoveSigner
  //============================================================================

  outcome::result<void> RemoveSigner::removeSigner(
      const RemoveSigner::Params &params, State &state, const Address &signer) {
    if (params.decrease_threshold) {
      if (state.threshold < 2) {
        return VMExitCode::kErrIllegalState;
      }
      state.threshold--;
    }

    state.signers.erase(
        std::find(state.signers.begin(), state.signers.end(), signer));
    return outcome::success();
  }

  outcome::result<RemoveSigner::Result> RemoveSigner::execute(
      Runtime &runtime,
      const RemoveSigner::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(resolved_signer,
                utils.getResolvedAddress(runtime, params.signer));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(
        v0::multisig::RemoveSigner::checkState(params, state, resolved_signer));
    OUTCOME_TRY(removeSigner(params, state, resolved_signer));
    OUTCOME_TRY(utils.purgeApprovals(state, resolved_signer));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveSigner) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // SwapSigner
  //============================================================================

  outcome::result<SwapSigner::Result> SwapSigner::execute(
      Runtime &runtime,
      const SwapSigner::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(from_resolved, utils.getResolvedAddress(runtime, params.from));
    OUTCOME_TRY(to_resolved, utils.getResolvedAddress(runtime, params.to));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(v0::multisig::SwapSigner::swapSigner(
        state, from_resolved, to_resolved));
    OUTCOME_TRY(utils.purgeApprovals(state, from_resolved));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SwapSigner) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // ChangeThreshold
  //============================================================================

  outcome::result<ChangeThreshold::Result> ChangeThreshold::execute(
      Runtime &runtime,
      const ChangeThreshold::Params &params,
      const MultisigUtils &utils) {
    return v0::multisig::ChangeThreshold::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(ChangeThreshold) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
  }

  // LockBalance
  //============================================================================

  outcome::result<void> LockBalance::checkAmount(
      const Runtime &runtime, const LockBalance::Params &params) {
    if ((runtime.getNetworkVersion() >= NetworkVersion::kVersion7)
        && (params.amount < 0)) {
      return VMExitCode::kErrIllegalArgument;
    }
    return outcome::success();
  }

  outcome::result<LockBalance::Result> LockBalance::execute(
      Runtime &runtime,
      const LockBalance::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(v0::multisig::LockBalance::checkUnlockDuration(params));
    OUTCOME_TRY(checkAmount(runtime, params));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(v0::multisig::LockBalance::lockBalance(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(LockBalance) {
    const MultisigUtilsImplV2 utils;
    return execute(runtime, params, utils);
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
