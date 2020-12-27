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

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(v0::multisig::Construct::checkCaller(runtime));
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

  // Propose
  //============================================================================

  ACTOR_METHOD_IMPL(Propose) {
    const MultisigUtilsImplV2 utils;

    OUTCOME_TRY(utils.assertCallerIsSignable(runtime));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(utils.assertCallerIsSigner(runtime, state));
    OUTCOME_TRY(tx, v0::multisig::Propose::createTransaction(params, state));
    auto &[tx_id, transaction] = tx;
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(approve, utils.approveTransaction(runtime, tx_id, transaction));
    const auto &[applied, return_value, code] = approve;

    return Result{tx_id, applied, code, return_value};
  }

  // Approve
  //============================================================================

  ACTOR_METHOD_IMPL(Approve) {
    const MultisigUtilsImplV2 utils;

    OUTCOME_TRY(utils.assertCallerIsSignable(runtime));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(utils.assertCallerIsSigner(runtime, state));

    return v0::multisig::Approve::approveTransaction(
        runtime, params, state, utils);
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

  ACTOR_METHOD_IMPL(AddSigner) {
    const MultisigUtilsImplV2 utils;

    OUTCOME_TRY(utils.assertCallerIsReceiver(runtime));
    OUTCOME_TRY(resolved_signer,
                utils.getResolvedAddress(runtime, params.signer));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(checkSignersCount(state.signers));
    OUTCOME_TRY(
        v0::multisig::AddSigner::addSigner(params, state, resolved_signer));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
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

  ACTOR_METHOD_IMPL(RemoveSigner) {
    const MultisigUtilsImplV2 utils;

    OUTCOME_TRY(utils.assertCallerIsReceiver(runtime));
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

  // SwapSigner
  //============================================================================

  ACTOR_METHOD_IMPL(SwapSigner) {
    const MultisigUtilsImplV2 utils;

    OUTCOME_TRY(utils.assertCallerIsReceiver(runtime));
    OUTCOME_TRY(from_resolved, utils.getResolvedAddress(runtime, params.from));
    OUTCOME_TRY(to_resolved, utils.getResolvedAddress(runtime, params.to));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(v0::multisig::SwapSigner::swapSigner(
        state, from_resolved, to_resolved));
    OUTCOME_TRY(utils.purgeApprovals(state, from_resolved));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // LockBalance
  //============================================================================

  outcome::result<void> LockBalance::checkNetwork(
      const Runtime &runtime, const LockBalance::Params &params) {
    if ((runtime.getNetworkVersion() >= NetworkVersion::kVersion7)
        && (params.amount < 0)) {
      return VMExitCode::kErrIllegalState;
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(LockBalance) {
    const MultisigUtilsImplV2 utils;

    OUTCOME_TRY(utils.assertCallerIsReceiver(runtime));
    OUTCOME_TRY(v0::multisig::LockBalance::checkUnlockDuration(params));
    OUTCOME_TRY(checkNetwork(runtime, params));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(v0::multisig::LockBalance::lockBalance(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

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
