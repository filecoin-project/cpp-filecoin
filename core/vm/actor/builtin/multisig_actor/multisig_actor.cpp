/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "multisig_actor.hpp"

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor_method.hpp"

using fc::common::Buffer;
using fc::primitives::BigInt;
using fc::vm::VMExitCode;
using fc::vm::actor::decodeActorParams;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::builtin::MultiSigActor;
using fc::vm::actor::builtin::MultiSignatureActorState;
using fc::vm::actor::builtin::MultiSignatureTransaction;
using fc::vm::runtime::InvocationOutput;

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
    return VMExitCode::MULTISIG_ACTOR_TRANSACTION_NOT_FOUND;

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
    return VMExitCode::MULTISIG_ACTOR_TRANSACTION_NOT_FOUND;

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
    return VMExitCode::MULTISIG_ACTOR_TRANSACTION_NOT_FOUND;

  pending_transactions.erase(pending_tx);
  return fc::outcome::success();
}

fc::outcome::result<void> MultiSignatureActorState::approveTransaction(
    const Actor &actor, Runtime &runtime, const TransactionNumber &tx_number) {
  Address caller = runtime.getImmediateCaller();
  if (!isSigner(caller)) return VMExitCode::MULTISIG_ACTOR_NOT_SIGNER;

  OUTCOME_TRY(pending_tx, getPendingTransaction(tx_number));

  if (std::find(pending_tx.approved.begin(), pending_tx.approved.end(), caller)
      != pending_tx.approved.end())
    return VMExitCode::MULTISIG_ACTOR_ALREADY_SIGNED;
  pending_tx.approved.push_back(caller);

  // check threshold
  if (pending_tx.approved.size() >= threshold) {
    if (actor.balance < pending_tx.value)
      return VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUND;

    auto amount_locked = getAmountLocked(runtime.getCurrentEpoch().toUInt64());
    if (actor.balance - pending_tx.value < amount_locked)
      return VMExitCode::MULTISIG_ACTOR_FUNDS_LOCKED;

    // send messsage ignoring value returned
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

fc::outcome::result<InvocationOutput> MultiSigActor::construct(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (runtime.getImmediateCaller() != kInitAddress)
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(construct_params,
              decodeActorParams<ConstructParameteres>(params));

  MultiSignatureActorState state{construct_params.signers,
                                 construct_params.threshold,
                                 TransactionNumber{0},
                                 BigInt{0},
                                 runtime.getCurrentEpoch().toUInt64(),
                                 construct_params.unlock_duration,
                                 {}};
  if (construct_params.unlock_duration != 0) {
    state.initial_balance = runtime.getValueReceived();
  }

  // commit state
  OUTCOME_TRY(state_cid, runtime.getIpfsDatastore()->setCbor(state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> MultiSigActor::propose(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (!isSignableActor(actor.code))
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(propose_params, decodeActorParams<ProposeParameters>(params));
  OUTCOME_TRY(state,
              runtime.getIpfsDatastore()->getCbor<MultiSignatureActorState>(
                  actor.head));

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
  OUTCOME_TRY(state.approveTransaction(actor, runtime, tx_number));

  // commit state
  OUTCOME_TRY(state_cid, runtime.getIpfsDatastore()->setCbor(state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  OUTCOME_TRY(encoded_result, codec::cbor::encode(tx_number));
  return InvocationOutput{Buffer{encoded_result}};
}

fc::outcome::result<InvocationOutput> MultiSigActor::approve(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (!isSignableActor(actor.code))
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(tx_params,
              decodeActorParams<TransactionNumberParameters>(params));
  OUTCOME_TRY(state,
              runtime.getIpfsDatastore()->getCbor<MultiSignatureActorState>(
                  actor.head));

  OUTCOME_TRY(
      state.approveTransaction(actor, runtime, tx_params.transaction_number));

  // commit state
  OUTCOME_TRY(state_cid, runtime.getIpfsDatastore()->setCbor(state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> MultiSigActor::cancel(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (!isSignableActor(actor.code))
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(tx_params,
              decodeActorParams<TransactionNumberParameters>(params));
  OUTCOME_TRY(state,
              runtime.getIpfsDatastore()->getCbor<MultiSignatureActorState>(
                  actor.head));
  Address caller = runtime.getImmediateCaller();
  if (!state.isSigner(caller)) return VMExitCode::MULTISIG_ACTOR_NOT_SIGNER;

  OUTCOME_TRY(is_tx_creator,
              state.isTransactionCreator(tx_params.transaction_number, caller));
  if (is_tx_creator) {
    OUTCOME_TRY(state.deletePendingTransaction(tx_params.transaction_number));
  } else {
    return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
  }

  // commit state
  OUTCOME_TRY(state_cid, runtime.getIpfsDatastore()->setCbor(state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> MultiSigActor::add_signer(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  OUTCOME_TRY(add_signer_params,
              decodeActorParams<AddSignerParameters>(params));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> MultiSigActor::remove_signer(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  OUTCOME_TRY(remove_signer_params,
              decodeActorParams<RemoveSignerParameters>(params));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> MultiSigActor::swap_signer(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  OUTCOME_TRY(swap_signer_params,
              decodeActorParams<SwapSignerParameters>(params));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> MultiSigActor::change_threshold(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  OUTCOME_TRY(change_threshold_params,
              decodeActorParams<ChangeTresholdParameters>(params));

  return fc::outcome::success();
}
