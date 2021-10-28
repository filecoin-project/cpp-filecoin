/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"

#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::multisig {
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using toolchain::Toolchain;

  // Construct
  //============================================================================

  outcome::result<std::vector<Address>> Construct::getResolvedSigners(
      Runtime &runtime, const std::vector<Address> &signers) {
    std::vector<Address> resolved_signers;
    for (const auto &signer : signers) {
      REQUIRE_NO_ERROR_A(resolved,
                         runtime.resolveOrCreate(signer),
                         VMExitCode::kErrIllegalState);
      const auto duplicate =
          std::find(resolved_signers.begin(), resolved_signers.end(), resolved);
      VALIDATE_ARG(duplicate == resolved_signers.end());
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

  MultisigActorStatePtr Construct::createState(
      Runtime &runtime, size_t threshold, const std::vector<Address> &signers) {
    MultisigActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    state->signers = signers;
    state->threshold = threshold;

    return state;
  }

  void Construct::setLocked(const Runtime &runtime,
                            const EpochDuration &unlock_duration,
                            MultisigActorStatePtr &state) {
    if (unlock_duration != 0) {
      state->setLocked(runtime.getCurrentEpoch(),
                       unlock_duration,
                       runtime.getValueReceived());
    }
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    VALIDATE_ARG(!params.signers.empty());
    OUTCOME_TRY(resolved_signers, getResolvedSigners(runtime, params.signers));
    OUTCOME_TRY(
        checkParams(params.signers, params.threshold, params.unlock_duration));

    auto state = createState(runtime, params.threshold, resolved_signers);
    state->signers = resolved_signers;
    state->threshold = params.threshold;

    setLocked(runtime, params.unlock_duration, state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // Propose
  //============================================================================

  ACTOR_METHOD_IMPL(Propose) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(utils->assertCallerIsSigner(state));

    const auto tx_id = state->next_transaction_id;
    state->next_transaction_id++;

    Transaction transaction{
        params.to, params.value, params.method, params.params, {}};
    REQUIRE_NO_ERROR(state->pending_transactions.set(tx_id, transaction),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(approve, utils->approveTransaction(tx_id, transaction));
    const auto &[applied, return_value, code] = approve;

    return Result{tx_id, applied, code, return_value};
  }

  // Approve
  //============================================================================

  ACTOR_METHOD_IMPL(Approve) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(utils->assertCallerIsSigner(state));

    // We need a copy to avoid the Flush of original state
    auto state_copy = state.copy();
    OUTCOME_TRY(transaction,
                state_copy->getTransaction(
                    runtime, params.tx_id, params.proposal_hash));
    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(execute,
                utils->executeTransaction(state, params.tx_id, transaction));
    auto [applied, return_value, code] = execute;

    if (!applied) {
      OUTCOME_TRY(approve,
                  utils->approveTransaction(params.tx_id, transaction));
      std::tie(applied, return_value, code) = approve;
    }

    return Result{applied, code, return_value};
  }

  // Cancel
  //============================================================================

  ACTOR_METHOD_IMPL(Cancel) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(utils->assertCallerIsSigner(state));

    REQUIRE_NO_ERROR_A(transaction,
                       state->getPendingTransaction(params.tx_id),
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

    REQUIRE_NO_ERROR(state->pending_transactions.remove(params.tx_id),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // AddSigner
  //============================================================================

  outcome::result<void> AddSigner::addSigner(const AddSigner::Params &params,
                                             MultisigActorStatePtr &state,
                                             const Address &signer) {
    if (state->isSigner(signer)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    state->signers.push_back(signer);
    if (params.increase_threshold) {
      state->threshold++;
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(resolved_signer, utils->getResolvedAddress(params.signer));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(addSigner(params, state, resolved_signer));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // RemoveSigner
  //============================================================================

  outcome::result<void> RemoveSigner::checkState(
      const RemoveSigner::Params &params,
      const MultisigActorStatePtr &state,
      const Address &signer) {
    if (!state->isSigner(signer)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (state->signers.size() == 1) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (!params.decrease_threshold
        && ((state->signers.size() - 1) < state->threshold)) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveSigner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(resolved_signer, utils->getResolvedAddress(params.signer));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(checkState(params, state, resolved_signer));

    if (params.decrease_threshold) {
      state->threshold--;
    }

    state->signers.erase(std::find(
        state->signers.begin(), state->signers.end(), resolved_signer));

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // SwapSigner
  //============================================================================

  outcome::result<void> SwapSigner::swapSigner(MultisigActorStatePtr &state,
                                               const Address &from,
                                               const Address &to) {
    if (!state->isSigner(from)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (state->isSigner(to)) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    state->signers.erase(
        std::remove(state->signers.begin(), state->signers.end(), from),
        state->signers.end());
    state->signers.push_back(to);
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SwapSigner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());

    const auto utils = Toolchain::createMultisigActorUtils(runtime);
    OUTCOME_TRY(from_resolved, utils->getResolvedAddress(params.from));
    OUTCOME_TRY(to_resolved, utils->getResolvedAddress(params.to));
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(swapSigner(state, from_resolved, to_resolved));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // ChangeThreshold
  //============================================================================

  ACTOR_METHOD_IMPL(ChangeThreshold) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());

    if ((params.new_threshold == 0)
        || (params.new_threshold > state->signers.size())) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    state->threshold = params.new_threshold;

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // LockBalance
  //============================================================================

  outcome::result<void> LockBalance::lockBalance(
      const LockBalance::Params &params, MultisigActorStatePtr &state) {
    if (state->unlock_duration != 0) {
      ABORT(VMExitCode::kErrForbidden);
    }
    state->setLocked(params.start_epoch, params.unlock_duration, params.amount);
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(LockBalance) {
    // This method was introduced at network version 2 in testnet.
    // Prior to that, the method did not exist so the VM would abort.
    if (runtime.getNetworkVersion() < NetworkVersion::kVersion2) {
      ABORT(VMExitCode::kSysErrInvalidMethod);
    }

    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    VALIDATE_ARG(params.unlock_duration > 0);
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
    OUTCOME_TRY(lockBalance(params, state));
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
}  // namespace fc::vm::actor::builtin::v0::multisig
