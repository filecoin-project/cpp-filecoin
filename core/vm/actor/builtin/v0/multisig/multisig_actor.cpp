/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v0/multisig/impl/multisig_utils_impl_v0.hpp"

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

  // Construct
  //============================================================================

  outcome::result<std::vector<Address>> Construct::getResolvedSigners(
      const Runtime &runtime, const std::vector<Address> &signers) {
    std::vector<Address> resolved_signers;
    for (const auto &signer : signers) {
      REQUIRE_NO_ERROR_A(resolved,
                         runtime.resolveAddress(signer),
                         VMExitCode::kErrIllegalState);
      const auto duplicate =
          std::find(resolved_signers.begin(), resolved_signers.end(), resolved);
      OUTCOME_TRY(
          runtime.validateArgument(duplicate == resolved_signers.end()));
      resolved_signers.push_back(resolved);
    }
    return std::move(resolved_signers);
  }

  outcome::result<void> Construct::checkParams(
      const std::vector<Address> &signers,
      size_t threshold,
      const EpochDuration &unlock_duration) {
    if (threshold > signers.size()) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    if (threshold < 1) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    if (unlock_duration < 0) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

  State Construct::createState(Runtime &runtime,
                               size_t threshold,
                               const std::vector<Address> &signers) {
    State state{signers,
                threshold,
                TransactionId{0},
                BigInt{0},
                ChainEpoch{0},
                EpochDuration{0},
                {}};

    IpldPtr {
      runtime
    }
    ->load(state);

    return state;
  }

  void Construct::setLocked(const Runtime &runtime,
                            const EpochDuration &unlock_duration,
                            State &state) {
    if (unlock_duration != 0) {
      state.setLocked(runtime.getCurrentEpoch(),
                      unlock_duration,
                      runtime.getValueReceived());
    }
  }

  outcome::result<Construct::Result> Construct::execute(
      Runtime &runtime,
      const Construct::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    OUTCOME_TRY(runtime.validateArgument(!params.signers.empty()));
    OUTCOME_TRY(resolved_signers, getResolvedSigners(runtime, params.signers));
    OUTCOME_TRY(
        checkParams(params.signers, params.threshold, params.unlock_duration));
    State state = createState(runtime, params.threshold, resolved_signers);
    setLocked(runtime, params.unlock_duration, state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // Propose
  //============================================================================

  outcome::result<std::tuple<TransactionId, Transaction>>
  Propose::createTransaction(const Propose::Params &params, State &state) {
    const auto tx_id = state.next_transaction_id;
    state.next_transaction_id++;

    Transaction transaction{
        params.to, params.value, params.method, params.params, {}};
    REQUIRE_NO_ERROR(state.pending_transactions.set(tx_id, transaction),
                     VMExitCode::kErrIllegalState);

    return std::make_tuple(tx_id, transaction);
  }

  outcome::result<Propose::Result> Propose::execute(
      Runtime &runtime,
      const Propose::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(utils.assertCallerIsSigner(runtime, state));
    OUTCOME_TRY(tx, createTransaction(params, state));
    auto &[tx_id, transaction] = tx;
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(approve, utils.approveTransaction(runtime, tx_id, transaction));
    const auto &[applied, return_value, code] = approve;

    return Result{tx_id, applied, code, return_value};
  }

  ACTOR_METHOD_IMPL(Propose) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // Approve
  //============================================================================

  outcome::result<Approve::Result> Approve::approveTransaction(
      Runtime &runtime,
      const Approve::Params &params,
      State &state,
      const MultisigUtils &utils) {
    // We need a copy to avoid the Flush of original state
    const auto state_copy{state};
    OUTCOME_TRY(
        transaction,
        state_copy.getTransaction(runtime, params.tx_id, params.proposal_hash));
    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(
        execute,
        utils.executeTransaction(runtime, state, params.tx_id, transaction));
    auto [applied, return_value, code] = execute;

    if (!applied) {
      OUTCOME_TRY(approve,
                  utils.approveTransaction(runtime, params.tx_id, transaction));
      std::tie(applied, return_value, code) = approve;
    }

    return Approve::Result{applied, code, return_value};
  }

  outcome::result<Approve::Result> Approve::execute(
      Runtime &runtime,
      const Approve::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(utils.assertCallerIsSigner(runtime, state));

    return approveTransaction(runtime, params, state, utils);
  }

  ACTOR_METHOD_IMPL(Approve) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // Cancel
  //============================================================================

  outcome::result<void> Cancel::checkTransaction(Runtime &runtime,
                                                 const Cancel::Params &params,
                                                 const State &state) {
    REQUIRE_NO_ERROR_A(transaction,
                       state.getPendingTransaction(params.tx_id),
                       VMExitCode::kErrNotFound);

    const auto caller = runtime.getImmediateCaller();
    const auto proposer =
        !transaction.approved.empty() ? transaction.approved[0] : Address{};
    if (proposer != caller) {
      ABORT(VMExitCode::kErrForbidden);
    }

    REQUIRE_NO_ERROR_A(
        hash, transaction.hash(runtime), VMExitCode::kErrIllegalState);

    if (!params.proposal_hash.empty() && (params.proposal_hash != hash)) {
      ABORT(VMExitCode::kErrIllegalState);
    }
    return outcome::success();
  }

  outcome::result<void> Cancel::removeTransaction(const Cancel::Params &params,
                                                  State &state) {
    REQUIRE_NO_ERROR(state.pending_transactions.remove(params.tx_id),
                     VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  outcome::result<Cancel::Result> Cancel::execute(Runtime &runtime,
                                                  const Cancel::Params &params,
                                                  const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(utils.assertCallerIsSigner(runtime, state));
    OUTCOME_TRY(checkTransaction(runtime, params, state));
    OUTCOME_TRY(removeTransaction(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Cancel) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // AddSigner
  //============================================================================

  outcome::result<void> AddSigner::addSigner(const AddSigner::Params &params,
                                             State &state,
                                             const Address &signer) {
    if (state.isSigner(signer)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    state.signers.push_back(signer);
    if (params.increase_threshold) {
      state.threshold++;
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
    OUTCOME_TRY(addSigner(params, state, resolved_signer));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // RemoveSigner
  //============================================================================

  outcome::result<void> RemoveSigner::checkState(
      const RemoveSigner::Params &params,
      const State &state,
      const Address &signer) {
    if (!state.isSigner(signer)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (state.signers.size() == 1) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (!params.decrease_threshold
        && ((state.signers.size() - 1) < state.threshold)) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

  void RemoveSigner::removeSigner(const RemoveSigner::Params &params,
                                  State &state,
                                  const Address &signer) {
    if (params.decrease_threshold) {
      state.threshold--;
    }

    state.signers.erase(
        std::find(state.signers.begin(), state.signers.end(), signer));
  }

  outcome::result<RemoveSigner::Result> RemoveSigner::execute(
      Runtime &runtime,
      const RemoveSigner::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(resolved_signer,
                utils.getResolvedAddress(runtime, params.signer));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(checkState(params, state, resolved_signer));
    removeSigner(params, state, resolved_signer);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveSigner) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // SwapSigner
  //============================================================================

  outcome::result<void> SwapSigner::swapSigner(State &state,
                                               const Address &from,
                                               const Address &to) {
    if (!state.isSigner(from)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (state.isSigner(to)) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    std::replace(state.signers.begin(), state.signers.end(), from, to);
    return outcome::success();
  }

  outcome::result<SwapSigner::Result> SwapSigner::execute(
      Runtime &runtime,
      const SwapSigner::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(from_resolved, utils.getResolvedAddress(runtime, params.from));
    OUTCOME_TRY(to_resolved, utils.getResolvedAddress(runtime, params.to));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(swapSigner(state, from_resolved, to_resolved));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SwapSigner) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // ChangeThreshold
  //============================================================================

  outcome::result<void> ChangeThreshold::changeThreshold(
      const ChangeThreshold::Params &params, State &state) {
    if ((params.new_threshold == 0)
        || (params.new_threshold > state.signers.size())) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    state.threshold = params.new_threshold;
    return outcome::success();
  }

  outcome::result<ChangeThreshold::Result> ChangeThreshold::execute(
      Runtime &runtime,
      const ChangeThreshold::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(changeThreshold(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangeThreshold) {
    const MultisigUtilsImplV0 utils;
    return execute(runtime, params, utils);
  }

  // LockBalance
  //============================================================================

  outcome::result<void> LockBalance::checkNetwork(const Runtime &runtime) {
    // This method was introduced at network version 2 in testnet.
    // Prior to that, the method did not exist so the VM would abort.
    if (runtime.getNetworkVersion() < NetworkVersion::kVersion2) {
      ABORT(VMExitCode::kSysErrInvalidMethod);
    }
    return outcome::success();
  }

  outcome::result<void> LockBalance::lockBalance(
      const LockBalance::Params &params, State &state) {
    if (state.unlock_duration != 0) {
      ABORT(VMExitCode::kErrForbidden);
    }
    state.setLocked(params.start_epoch, params.unlock_duration, params.amount);
    return outcome::success();
  }

  outcome::result<LockBalance::Result> LockBalance::execute(
      Runtime &runtime,
      const LockBalance::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(checkNetwork(runtime));
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(runtime.validateArgument(params.unlock_duration > 0));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(lockBalance(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(LockBalance) {
    const MultisigUtilsImplV0 utils;
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
}  // namespace fc::vm::actor::builtin::v0::multisig
