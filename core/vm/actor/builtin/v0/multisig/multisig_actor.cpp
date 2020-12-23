/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"

namespace fc::vm::actor::builtin::v0::multisig {

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

  outcome::result<void> assertCallerIsSignable(Runtime &runtime) {
    OUTCOME_TRY(code, runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (!isSignableActor(code)) {
      return VMExitCode::kSysErrForbidden;
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kInitAddress) {
      return VMExitCode::kSysErrForbidden;
    }

    if (params.signers.empty()) {
      return VMExitCode::kErrIllegalArgument;
    }

    std::vector<Address> resolved_signers;
    for (const auto &signer : params.signers) {
      const auto resolved = runtime.resolveAddress(signer);
      if (resolved.has_error()) {
        return VMExitCode::kErrIllegalState;
      }
      const auto duplicate = std::find(
          resolved_signers.begin(), resolved_signers.end(), resolved.value());
      if (duplicate != resolved_signers.end()) {
        return VMExitCode::kErrIllegalArgument;
      }
      resolved_signers.push_back(resolved.value());
    }

    if (params.threshold > params.signers.size()) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (params.threshold < 1) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (params.unlock_duration < 0) {
      return VMExitCode::kErrIllegalArgument;
    }

    State state{resolved_signers,
                params.threshold,
                TransactionId{0},
                BigInt{0},
                ChainEpoch{0},
                EpochDuration{0},
                {}};

    IpldPtr {
      runtime
    }
    ->load(state);

    if (params.unlock_duration != 0) {
      state.setLocked(runtime.getCurrentEpoch(),
                      params.unlock_duration,
                      runtime.getValueReceived());
    }

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(Propose) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));

    const auto proposer = runtime.getImmediateCaller();
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (!state.isSigner(proposer)) {
      return VMExitCode::kErrForbidden;
    }

    const auto tx_id = state.next_transaction_id;
    state.next_transaction_id++;

    Transaction transaction{
        params.to, params.value, params.method, params.params, {}};
    const auto result = state.pending_transactions.set(tx_id, transaction);
    if (result.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(approve, approveTransaction(runtime, tx_id, transaction));
    const auto &[applied, return_value, code] = approve;

    return Result{tx_id, applied, code, return_value};
  }

  ACTOR_METHOD_IMPL(Approve) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));

    const auto caller = runtime.getImmediateCaller();
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (!state.isSigner(caller)) {
      return VMExitCode::kErrForbidden;
    }

    // We need a copy to avoid the Flush of original state
    const auto state_copy{state};
    OUTCOME_TRY(
        transaction,
        state_copy.getTransaction(runtime, params.tx_id, params.proposal_hash));
    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(execute,
                executeTransaction(runtime, state, params.tx_id, transaction));
    auto [applied, return_value, code] = execute;

    if (!applied) {
      OUTCOME_TRY(approve,
                  approveTransaction(runtime, params.tx_id, transaction));
      std::tie(applied, return_value, code) = approve;
    }

    return Result{applied, code, return_value};
  }

  ACTOR_METHOD_IMPL(Cancel) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));

    const auto caller = runtime.getImmediateCaller();
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (!state.isSigner(caller)) {
      return VMExitCode::kErrForbidden;
    }

    const auto transaction = state.getPendingTransaction(params.tx_id);
    if (transaction.has_error()) {
      return VMExitCode::kErrNotFound;
    }

    const auto proposer = !transaction.value().approved.empty()
                              ? transaction.value().approved[0]
                              : Address{};
    if (proposer != caller) {
      return VMExitCode::kErrForbidden;
    }

    const auto hash = transaction.value().hash(runtime);
    if (hash.has_error()) {
      return VMExitCode::kErrIllegalState;
    }
    if (!params.proposal_hash.empty()
        && (params.proposal_hash != hash.value())) {
      return VMExitCode::kErrIllegalState;
    }

    const auto result = state.pending_transactions.remove(params.tx_id);
    if (result.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::kSysErrForbidden;
    }

    const auto resolved_signer = runtime.resolveAddress(params.signer);
    if (resolved_signer.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (state.isSigner(resolved_signer.value())) {
      return VMExitCode::kErrForbidden;
    }

    state.signers.push_back(resolved_signer.value());
    if (params.increase_threshold) {
      state.threshold++;
    }

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveSigner) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::kSysErrForbidden;
    }

    const auto resolved_signer = runtime.resolveAddress(params.signer);
    if (resolved_signer.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    const auto [is_signer, it] = state.checkSigner(resolved_signer.value());
    if (!is_signer) {
      return VMExitCode::kErrForbidden;
    }

    if (state.signers.size() == 1) {
      return VMExitCode::kErrForbidden;
    }

    if (!params.decrease_threshold
        && ((state.signers.size() - 1) < state.threshold)) {
      return VMExitCode::kErrIllegalArgument;
    }

    if (params.decrease_threshold) {
      state.threshold--;
    }

    state.signers.erase(it);

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(SwapSigner) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::kSysErrForbidden;
    }

    const auto from_resolved = runtime.resolveAddress(params.from);
    if (from_resolved.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    const auto to_resolved = runtime.resolveAddress(params.to);
    if (to_resolved.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (!state.isSigner(from_resolved.value())) {
      return VMExitCode::kErrForbidden;
    }

    if (state.isSigner(to_resolved.value())) {
      return VMExitCode::kErrIllegalArgument;
    }

    std::replace(state.signers.begin(),
                 state.signers.end(),
                 from_resolved.value(),
                 to_resolved.value());

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangeThreshold) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::kSysErrForbidden;
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if ((params.new_threshold == 0)
        || (params.new_threshold > state.signers.size())) {
      return VMExitCode::kErrIllegalArgument;
    }

    state.threshold = params.new_threshold;

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(LockBalance) {
    // This method was introduced at network version 2 in testnet.
    // Prior to that, the method did not exist so the VM would abort.
    if (runtime.getNetworkVersion() < NetworkVersion::kVersion2) {
      return VMExitCode::kSysErrInvalidMethod;
    }

    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::kSysErrForbidden;
    }

    if (params.unlock_duration <= 0) {
      return VMExitCode::kErrIllegalArgument;
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (state.unlock_duration != 0) {
      return VMExitCode::kErrForbidden;
    }

    state.setLocked(params.start_epoch, params.unlock_duration, params.amount);

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
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
}  // namespace fc::vm::actor::builtin::v0::multisig
