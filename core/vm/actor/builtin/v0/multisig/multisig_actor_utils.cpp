/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/multisig/multisig_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::multisig {

  outcome::result<void> MultisigUtils::assertCallerIsSigner(
      const MultisigActorStatePtr &state) const {
    const auto proposer = getRuntime().getImmediateCaller();
    if (!state->isSigner(proposer)) {
      ABORT(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }

  outcome::result<Address> MultisigUtils::getResolvedAddress(
      const Address &address) const {
    REQUIRE_NO_ERROR_A(resolved,
                       getRuntime().resolveOrCreate(address),
                       VMExitCode::kErrIllegalState);
    return std::move(resolved);
  }

  BigInt MultisigUtils::amountLocked(const MultisigActorStatePtr &state,
                                     const ChainEpoch &elapsed_epoch) const {
    if (elapsed_epoch >= state->unlock_duration) {
      return 0;
    }
    if (elapsed_epoch < 0) {
      return state->initial_balance;
    }

    const auto unit_locked =
        bigdiv(state->initial_balance, state->unlock_duration);
    return unit_locked * (state->unlock_duration - elapsed_epoch);
  }

  outcome::result<void> MultisigUtils::assertAvailable(
      const MultisigActorStatePtr &state,
      const TokenAmount &current_balance,
      const TokenAmount &amount_to_spend,
      const ChainEpoch &current_epoch) const {
    if (amount_to_spend < 0) {
      ABORT(VMExitCode::kErrInsufficientFunds);
    }
    if (current_balance < amount_to_spend) {
      ABORT(VMExitCode::kErrInsufficientFunds);
    }

    const auto remaining_balance = current_balance - amount_to_spend;
    const auto amount_locked =
        amountLocked(state, current_epoch - state->start_epoch);
    if (remaining_balance < amount_locked) {
      ABORT(VMExitCode::kErrInsufficientFunds);
    }
    return outcome::success();
  }

  outcome::result<ApproveTransactionResult> MultisigUtils::approveTransaction(
      const TransactionId &tx_id, Transaction &transaction) const {
    const Address caller = getRuntime().getImmediateCaller();
    if (std::find(
            transaction.approved.begin(), transaction.approved.end(), caller)
        != transaction.approved.end()) {
      ABORT(VMExitCode::kErrForbidden);
    }
    transaction.approved.push_back(caller);

    OUTCOME_TRY(state, getRuntime().getActorState<MultisigActorStatePtr>());

    REQUIRE_NO_ERROR(state->pending_transactions.set(tx_id, transaction),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(getRuntime().commitState(state));

    return executeTransaction(state, tx_id, transaction);
  }

  outcome::result<ApproveTransactionResult> MultisigUtils::executeTransaction(
      MultisigActorStatePtr &state,
      const TransactionId &tx_id,
      const Transaction &transaction) const {
    bool applied = false;
    Buffer out{};
    VMExitCode code = VMExitCode::kOk;

    if (transaction.approved.size() >= state->threshold) {
      OUTCOME_TRY(balance, getRuntime().getCurrentBalance());
      OUTCOME_TRY(assertAvailable(
          state, balance, transaction.value, getRuntime().getCurrentEpoch()));

      const auto send_result = getRuntime().send(transaction.to,
                                                 transaction.method,
                                                 transaction.params,
                                                 transaction.value);
      OUTCOME_TRYA(code, asExitCode(send_result));
      if (send_result) {
        out = send_result.value();
      }
      applied = true;

      // Lotus gas conformance
      OUTCOME_TRYA(state, getRuntime().getActorState<MultisigActorStatePtr>());

      REQUIRE_NO_ERROR(state->pending_transactions.remove(tx_id),
                       VMExitCode::kErrIllegalState);
      OUTCOME_TRY(getRuntime().commitState(state));
    }

    return std::make_tuple(applied, out, code);
  }

}  // namespace fc::vm::actor::builtin::v0::multisig
