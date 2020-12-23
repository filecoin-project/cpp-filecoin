/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::multisig {

  bool Transaction::operator==(const Transaction &other) const {
    return to == other.to && value == other.value && method == other.method
           && params == other.params && approved == other.approved;
  }

  outcome::result<Buffer> Transaction::hash(Runtime &runtime) const {
    ProposalHashData hash_data(*this);
    OUTCOME_TRY(to_hash, codec::cbor::encode(hash_data));
    OUTCOME_TRY(hash, runtime.hashBlake2b(to_hash));
    return Buffer{gsl::make_span(hash)};
  }

  void State::setLocked(const ChainEpoch &start_epoch,
                        const EpochDuration &unlock_duration,
                        const TokenAmount &lockedAmount) {
    this->start_epoch = start_epoch;
    this->unlock_duration = unlock_duration;
    this->initial_balance = lockedAmount;
  }

  fc::outcome::result<Transaction> State::getPendingTransaction(
      const TransactionId &tx_id) const {
    OUTCOME_TRY(pending_tx, pending_transactions.tryGet(tx_id));
    if (!pending_tx) {
      return VMExitCode::kErrNotFound;
    }

    return std::move(pending_tx.get());
  }

  outcome::result<Transaction> State::getTransaction(
      Runtime &runtime,
      const TransactionId &tx_id,
      const Buffer &proposal_hash) const {
    OUTCOME_TRY(transaction, getPendingTransaction(tx_id));
    const auto hash = transaction.hash(runtime);
    if (hash.has_error()) {
      return VMExitCode::kErrIllegalState;
    }
    if (!proposal_hash.empty() && (proposal_hash != hash.value())) {
      return VMExitCode::kErrIllegalArgument;
    }

    return std::move(transaction);
  }

  BigInt State::amountLocked(const ChainEpoch &elapsed_epoch) const {
    if (elapsed_epoch >= unlock_duration) {
      return 0;
    }
    if (elapsed_epoch < 0) {
      return initial_balance;
    }

    const auto unit_locked = bigdiv(initial_balance, unlock_duration);
    return unit_locked * (unlock_duration - elapsed_epoch);
  }

  outcome::result<void> State::assertAvailable(
      const TokenAmount &current_balance,
      const TokenAmount &amount_to_spend,
      const ChainEpoch &current_epoch) const {
    if (amount_to_spend < 0) {
      return VMExitCode::kErrInsufficientFunds;
    }
    if (current_balance < amount_to_spend) {
      return VMExitCode::kErrInsufficientFunds;
    }

    const auto remaining_balance = current_balance - amount_to_spend;
    const auto amount_locked = amountLocked(current_epoch - start_epoch);
    if (remaining_balance < amount_locked) {
      return VMExitCode::kErrInsufficientFunds;
    }
    return fc::outcome::success();
  }

  outcome::result<ApproveTransactionResult> approveTransaction(
      Runtime &runtime, const TransactionId &tx_id, Transaction &transaction) {
    Address caller = runtime.getImmediateCaller();

    if (std::find(
            transaction.approved.begin(), transaction.approved.end(), caller)
        != transaction.approved.end()) {
      return VMExitCode::kErrForbidden;
    }
    transaction.approved.push_back(caller);

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    const auto result = state.pending_transactions.set(tx_id, transaction);
    if (result.has_error()) {
      return VMExitCode::kErrIllegalState;
    }

    OUTCOME_TRY(runtime.commitState(state));

    return executeTransaction(runtime, state, tx_id, transaction);
  }

  outcome::result<ApproveTransactionResult> executeTransaction(
      Runtime &runtime,
      State &state,
      const TransactionId &tx_id,
      const Transaction &transaction) {
    bool applied = false;
    Buffer out{};
    VMExitCode code = VMExitCode::kOk;

    if (transaction.approved.size() >= state.threshold) {
      OUTCOME_TRY(balance, runtime.getCurrentBalance());
      OUTCOME_TRY(state.assertAvailable(
          balance, transaction.value, runtime.getCurrentEpoch()));

      const auto send_result = runtime.send(transaction.to,
                                            transaction.method,
                                            transaction.params,
                                            transaction.value);
      if (send_result.has_error()) {
        if (!isVMExitCode(send_result.error())) {
          return send_result.error();
        }
        code = static_cast<VMExitCode>(send_result.error().value());
      } else {
        out = send_result.value();
      }
      applied = true;

      // Charge gas and get new state
      OUTCOME_TRY(new_state, runtime.getCurrentActorStateCbor<State>());

      const auto result = new_state.pending_transactions.remove(tx_id);
      if (result.has_error()) {
        return VMExitCode::kErrIllegalState;
      }
      OUTCOME_TRY(runtime.commitState(new_state));
    }

    return std::make_tuple(applied, out, code);
  }
}  // namespace fc::vm::actor::builtin::v0::multisig
