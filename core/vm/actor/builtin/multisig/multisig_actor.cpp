/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/multisig/multisig_actor.hpp"

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor_method.hpp"

using fc::common::Buffer;
using fc::primitives::BigInt;
using fc::primitives::ChainEpoch;
using fc::vm::VMExitCode;
using fc::vm::actor::ActorExports;
using fc::vm::actor::ActorMethod;
using fc::vm::actor::decodeActorParams;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::builtin::multisig::kAddSignerMethodNumber;
using fc::vm::actor::builtin::multisig::kApproveMethodNumber;
using fc::vm::actor::builtin::multisig::kCancelMethodNumber;
using fc::vm::actor::builtin::multisig::kChangeThresholdMethodNumber;
using fc::vm::actor::builtin::multisig::kProposeMethodNumber;
using fc::vm::actor::builtin::multisig::kRemoveSignerMethodNumber;
using fc::vm::actor::builtin::multisig::kSwapSignerMethodNumber;
using fc::vm::actor::builtin::multisig::MultiSigActor;
using fc::vm::actor::builtin::multisig::MultiSignatureActorState;
using fc::vm::actor::builtin::multisig::MultiSignatureTransaction;
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

  if (std::find(pending_tx.approved.begin(), pending_tx.approved.end(), caller)
      != pending_tx.approved.end())
    return VMExitCode::MULTISIG_ACTOR_ILLEGAL_STATE;
  pending_tx.approved.push_back(caller);

  // check threshold
  OUTCOME_TRY(balance, runtime.getBalance(runtime.getMessage().get().to));
  if (pending_tx.approved.size() >= threshold) {
    if (balance < pending_tx.value)
      return VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS;

    auto amount_locked = getAmountLocked(runtime.getCurrentEpoch());
    if (balance - pending_tx.value < amount_locked)
      return VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS;

    // send messsage ignoring value returned
    // https://github.com/filecoin-project/specs-actors/issues/113
    // NOLINTNEXTLINE(clang-diagnostic-unused-result)
    runtime.send(
        pending_tx.to, pending_tx.method, pending_tx.params, pending_tx.value);

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

ACTOR_METHOD(MultiSigActor::construct) {
  if (runtime.getImmediateCaller() != kInitAddress)
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(construct_params, decodeActorParams<ConstructParameters>(params));
  if (construct_params.signers.size() < construct_params.threshold)
    return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

  MultiSignatureActorState state{construct_params.signers,
                                 construct_params.threshold,
                                 TransactionNumber{0},
                                 BigInt{0},
                                 runtime.getCurrentEpoch(),
                                 construct_params.unlock_duration,
                                 {}};

  if (construct_params.unlock_duration != 0) {
    state.initial_balance = runtime.getValueReceived();
  }

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

ACTOR_METHOD(MultiSigActor::propose) {
  OUTCOME_TRY(assertCallerIsSignable(runtime));

  OUTCOME_TRY(propose_params, decodeActorParams<ProposeParameters>(params));
  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

  TransactionNumber tx_number = state.next_transaction_id;
  state.next_transaction_id++;

  MultiSignatureTransaction transaction{tx_number,
                                        propose_params.to,
                                        propose_params.value,
                                        propose_params.method,
                                        propose_params.params,
                                        {}};
  state.pending_transactions.push_back(transaction);

  // approve pending tx
  OUTCOME_TRY(state.approveTransaction(runtime, tx_number));

  OUTCOME_TRY(encoded_result, codec::cbor::encode(tx_number));

  OUTCOME_TRY(runtime.commitState(state));
  return InvocationOutput{Buffer{encoded_result}};
}

ACTOR_METHOD(MultiSigActor::approve) {
  OUTCOME_TRY(assertCallerIsSignable(runtime));

  OUTCOME_TRY(tx_params,
              decodeActorParams<TransactionNumberParameters>(params));
  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

  OUTCOME_TRY(state.approveTransaction(runtime, tx_params.transaction_number));

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

ACTOR_METHOD(MultiSigActor::cancel) {
  OUTCOME_TRY(assertCallerIsSignable(runtime));

  OUTCOME_TRY(tx_params,
              decodeActorParams<TransactionNumberParameters>(params));
  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());
  Address caller = runtime.getImmediateCaller();
  if (!state.isSigner(caller)) return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;

  OUTCOME_TRY(is_tx_creator,
              state.isTransactionCreator(tx_params.transaction_number, caller));
  if (is_tx_creator) {
    OUTCOME_TRY(state.deletePendingTransaction(tx_params.transaction_number));
  } else {
    return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;
  }

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

ACTOR_METHOD(MultiSigActor::addSigner) {
  if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
  }

  OUTCOME_TRY(add_signer_params,
              decodeActorParams<AddSignerParameters>(params));
  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

  if (state.isSigner(add_signer_params.signer))
    return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

  state.signers.push_back(add_signer_params.signer);
  if (add_signer_params.increase_threshold) state.threshold++;

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

ACTOR_METHOD(MultiSigActor::removeSigner) {
  if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
  }

  OUTCOME_TRY(remove_signer_params,
              decodeActorParams<RemoveSignerParameters>(params));
  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

  auto signer = std::find(
      state.signers.begin(), state.signers.end(), remove_signer_params.signer);
  if (signer == state.signers.end())
    return VMExitCode::MULTISIG_ACTOR_FORBIDDEN;
  state.signers.erase(signer);

  if (remove_signer_params.decrease_threshold) state.threshold--;

  // actor-spec ignores decrease_threshold parameters in this case and call it
  // automatic threshold decrease
  if (state.threshold < 1 || state.signers.size() < state.threshold)
    return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

ACTOR_METHOD(MultiSigActor::swapSigner) {
  if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
  }

  OUTCOME_TRY(swap_signer_params,
              decodeActorParams<SwapSignerParameters>(params));
  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());

  if (state.isSigner(swap_signer_params.new_signer))
    return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;
  auto old_signer = std::find(state.signers.begin(),
                              state.signers.end(),
                              swap_signer_params.old_signer);
  if (old_signer == state.signers.end())
    return VMExitCode::MULTISIG_ACTOR_NOT_FOUND;
  *old_signer = swap_signer_params.new_signer;

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

ACTOR_METHOD(MultiSigActor::changeThreshold) {
  if (runtime.getImmediateCaller() != runtime.getCurrentReceiver()) {
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
  }

  OUTCOME_TRY(change_threshold_params,
              decodeActorParams<ChangeThresholdParameters>(params));

  OUTCOME_TRY(state,
              runtime.getCurrentActorStateCbor<MultiSignatureActorState>());
  if (change_threshold_params.new_threshold == 0
      || change_threshold_params.new_threshold > state.signers.size())
    return VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT;

  state.threshold = change_threshold_params.new_threshold;

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

const ActorExports fc::vm::actor::builtin::multisig::exports = {
    {kProposeMethodNumber, ActorMethod(MultiSigActor::propose)},
    {kApproveMethodNumber, ActorMethod(MultiSigActor::approve)},
    {kCancelMethodNumber, ActorMethod(MultiSigActor::cancel)},
    {kAddSignerMethodNumber, ActorMethod(MultiSigActor::addSigner)},
    {kRemoveSignerMethodNumber, ActorMethod(MultiSigActor::removeSigner)},
    {kSwapSignerMethodNumber, ActorMethod(MultiSigActor::swapSigner)},
    {kChangeThresholdMethodNumber,
     ActorMethod(MultiSigActor::changeThreshold)}};
