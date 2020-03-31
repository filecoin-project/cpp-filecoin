/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/multisig/multisig_actor.hpp"

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::multisig {

  using fc::common::Buffer;
  using fc::primitives::BigInt;
  using fc::primitives::ChainEpoch;
  using fc::vm::VMExitCode;
  using fc::vm::actor::ActorExports;
  using fc::vm::actor::ActorMethod;
  using fc::vm::actor::decodeActorParams;
  using fc::vm::actor::kInitAddress;
  using fc::vm::runtime::InvocationOutput;
  using fc::vm::runtime::Runtime;
  namespace outcome = fc::outcome;

  outcome::result<void> assertCallerIsSignable(Runtime &runtime) {
    OUTCOME_TRY(code, runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (!isSignableActor(code)) {
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
    }
    return outcome::success();
  }

  bool MultiSignatureTransaction::operator==(
      const MultiSignatureTransaction &other) const {
    return transaction_number == other.transaction_number && to == other.to
           && value == other.value && method == other.method
           && params == other.params && approved == other.approved;
  }

  bool MultiSignatureActorState::isSigner(const Address &address) const {
    return std::find(signers.begin(), signers.end(), address) != signers.end();
  }

  fc::outcome::result<bool> MultiSignatureActorState::isTransactionCreator(
      const TransactionNumber &tx_number, const Address &address) const {
    OUTCOME_TRY(pending_tx, getPendingTransaction(tx_number));
    if (pending_tx.approved.empty()) return false;
    // Check tx creator is current caller. The first signer is tx creator.
    return pending_tx.approved[0] == address;
  }

  fc::outcome::result<MultiSignatureTransaction>
  MultiSignatureActorState::getPendingTransaction(
      const TransactionNumber &tx_number) const {
    auto pending_tx =
        std::find_if(pending_transactions.begin(),
                     pending_transactions.end(),
                     [&tx_number](const MultiSignatureTransaction &tx) {
                       return tx.transaction_number == tx_number;
                     });
    if (pending_tx == pending_transactions.end())
      return VMExitCode::MULTISIG_ACTOR_NOT_FOUND;

    return *pending_tx;
  }

  fc::outcome::result<void> MultiSignatureActorState::updatePendingTransaction(
      const MultiSignatureTransaction &transaction) {
    auto pending_tx = std::find_if(
        pending_transactions.begin(),
        pending_transactions.end(),
        [&transaction](const MultiSignatureTransaction &tx) {
          return tx.transaction_number == transaction.transaction_number;
        });
    if (pending_tx == pending_transactions.end())
      return VMExitCode::MULTISIG_ACTOR_NOT_FOUND;

    *pending_tx = transaction;
    return fc::outcome::success();
  }

  fc::outcome::result<void> MultiSignatureActorState::deletePendingTransaction(
      const TransactionNumber &tx_number) {
    auto pending_tx =
        std::find_if(pending_transactions.begin(),
                     pending_transactions.end(),
                     [&tx_number](const MultiSignatureTransaction &tx) {
                       return tx.transaction_number == tx_number;
                     });
    if (pending_tx == pending_transactions.end())
      return VMExitCode::MULTISIG_ACTOR_NOT_FOUND;

    pending_transactions.erase(pending_tx);
    return fc::outcome::success();
  }

  fc::outcome::result<void> MultiSignatureActorState::approveTransaction(
      Runtime &runtime, const TransactionNumber &tx_number) {
    Address caller = runtime.getImmediateCaller();
    if (!isSigner(caller)) return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;

    OUTCOME_TRY(pending_tx, getPendingTransaction(tx_number));

    if (std::find(
            pending_tx.approved.begin(), pending_tx.approved.end(), caller)
        != pending_tx.approved.end())
      return VMExitCode::MULTISIG_ACTOR_ILLEGAL_STATE;
    pending_tx.approved.push_back(caller);

    // check threshold
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    if (pending_tx.approved.size() >= threshold) {
      if (balance < pending_tx.value)
        return VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS;

      auto amount_locked = getAmountLocked(runtime.getCurrentEpoch());
      if (balance - pending_tx.value < amount_locked)
        return VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS;

      // send messsage ignoring value returned
      // https://github.com/filecoin-project/specs-actors/issues/113
      // NOLINTNEXTLINE(clang-diagnostic-unused-result)
      runtime.send(pending_tx.to,
                   pending_tx.method,
                   pending_tx.params,
                   pending_tx.value);

      OUTCOME_TRY(deletePendingTransaction(tx_number));
    } else {
      OUTCOME_TRY(updatePendingTransaction(pending_tx));
    }

    return fc::outcome::success();
  }

  BigInt MultiSignatureActorState::getAmountLocked(
      const ChainEpoch &current_epoch) const {
    if (current_epoch < start_epoch) return initial_balance;
    auto elapsed_epoch = current_epoch - start_epoch;
    if (unlock_duration < elapsed_epoch) return 0;
    return initial_balance / unlock_duration * elapsed_epoch;
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kInitAddress)
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;

    if (params.signers.size() < params.threshold)
      return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

    MultiSignatureActorState state{params.signers,
                                   params.threshold,
                                   TransactionNumber{0},
                                   BigInt{0},
                                   runtime.getCurrentEpoch(),
                                   params.unlock_duration,
                                   {}};

    if (params.unlock_duration != 0) {
      state.initial_balance = runtime.getValueReceived();
    }

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(Propose) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

    TransactionNumber tx_number = state.next_transaction_id;
    state.next_transaction_id++;

    MultiSignatureTransaction transaction{
        tx_number, params.to, params.value, params.method, params.params, {}};
    state.pending_transactions.push_back(transaction);

    // approve pending tx
    OUTCOME_TRY(state.approveTransaction(runtime, tx_number));

    OUTCOME_TRY(runtime.commitState(state));
    return tx_number;
  }

  ACTOR_METHOD_IMPL(Approve) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

    OUTCOME_TRY(state.approveTransaction(runtime, params.transaction_number));

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(Cancel) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());
    Address caller = runtime.getImmediateCaller();
    if (!state.isSigner(caller)) return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;

    OUTCOME_TRY(is_tx_creator,
                state.isTransactionCreator(params.transaction_number, caller));
    if (is_tx_creator) {
      OUTCOME_TRY(state.deletePendingTransaction(params.transaction_number));
    } else {
      return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;
    }

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
    }

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

    if (state.isSigner(params.signer))
      return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

    state.signers.push_back(params.signer);
    if (params.increase_threshold) state.threshold++;

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveSigner) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
    }

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

    auto signer =
        std::find(state.signers.begin(), state.signers.end(), params.signer);
    if (signer == state.signers.end())
      return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;
    state.signers.erase(signer);

    if (params.decrease_threshold) --state.threshold;

    // actor-spec ignores decrease_threshold parameters in this case and call it
    // automatic threshold decrease
    if (state.threshold < 1 || state.signers.size() < state.threshold)
      return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(SwapSigner) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
    }

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

    if (state.isSigner(params.new_signer))
      return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;
    auto old_signer = std::find(
        state.signers.begin(), state.signers.end(), params.old_signer);
    if (old_signer == state.signers.end())
      return VMExitCode::MULTISIG_ACTOR_NOT_FOUND;
    *old_signer = params.new_signer;

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangeThreshold) {
    if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
    }

    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<MultiSignatureActorState>());
    if (params.new_threshold == 0
        || params.new_threshold > state.signers.size())
      return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

    state.threshold = params.new_threshold;

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  const ActorExports exports = {
      exportMethod<Construct>(),
      exportMethod<Propose>(),
      exportMethod<Approve>(),
      exportMethod<Cancel>(),
      exportMethod<AddSigner>(),
      exportMethod<RemoveSigner>(),
      exportMethod<SwapSigner>(),
      exportMethod<ChangeThreshold>(),
  };
}  // namespace fc::vm::actor::builtin::multisig
