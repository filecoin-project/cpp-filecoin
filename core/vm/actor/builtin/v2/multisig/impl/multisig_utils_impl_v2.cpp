/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/multisig/impl/multisig_utils_impl_v2.hpp"

namespace fc::vm::actor::builtin::v2::multisig {

  BigInt MultisigUtilsImplV2::amountLocked(
      const State &state, const ChainEpoch &elapsed_epoch) const {
    if (elapsed_epoch >= state.unlock_duration) {
      return 0;
    }
    if (elapsed_epoch <= 0) {
      return state.initial_balance;
    }

    const auto remaining_duration = state.unlock_duration - elapsed_epoch;
    const auto numerator = state.initial_balance * remaining_duration;
    const auto quot = bigdiv(numerator, state.unlock_duration);
    const auto rem = bigmod(numerator, state.unlock_duration);

    const auto locked = (rem == 0) ? quot : quot + 1;
    return locked;
  }

  outcome::result<void> MultisigUtilsImplV2::assertAvailable(
      const State &state,
      const TokenAmount &current_balance,
      const TokenAmount &amount_to_spend,
      const ChainEpoch &current_epoch) const {
    if (amount_to_spend < 0) {
      return VMExitCode::kErrInsufficientFunds;
    }
    if (current_balance < amount_to_spend) {
      return VMExitCode::kErrInsufficientFunds;
    }
    if (amount_to_spend == 0) {
      // Always permit a transaction that sends no value, even if the lockup
      // exceeds the current balance.
      return outcome::success();
    }

    const auto remaining_balance = current_balance - amount_to_spend;
    const auto amount_locked =
        amountLocked(state, current_epoch - state.start_epoch);
    if (remaining_balance < amount_locked) {
      return VMExitCode::kErrInsufficientFunds;
    }
    return outcome::success();
  }

  outcome::result<ApproveTransactionResult>
  MultisigUtilsImplV2::executeTransaction(
      Runtime &runtime,
      State &state,
      const TransactionId &tx_id,
      const Transaction &transaction) const {
    bool applied = false;
    Buffer out{};
    VMExitCode code = VMExitCode::kOk;

    if (transaction.approved.size() >= state.threshold) {
      OUTCOME_TRY(balance, runtime.getCurrentBalance());
      OUTCOME_TRY(assertAvailable(
          state, balance, transaction.value, runtime.getCurrentEpoch()));

      const auto send_result = runtime.send(transaction.to,
                                            transaction.method,
                                            transaction.params,
                                            transaction.value);
      OUTCOME_TRYA(code, asExitCode(send_result));
      if (send_result) {
        out = send_result.value();
      }
      applied = true;

      // Lotus gas conformance
      OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());

      // Prior to version 6 we attempt to delete all transactions, even those
      // no longer in the pending txns map because they have been purged.
      bool should_delete = true;
      // Starting at version 6 we first check if the transaction exists before
      // deleting. This allows 1 out of n multisig swaps and removes initiated
      // by the swapped/removed signer to go through without an illegal state
      // error
      if (runtime.getNetworkVersion() >= NetworkVersion::kVersion6) {
        const auto tx_exists = state.pending_transactions.has(tx_id);
        REQUIRE_NO_ERROR(tx_exists, VMExitCode::kErrIllegalState);
        should_delete = tx_exists.value();
      }

      if (should_delete) {
        const auto result = state.pending_transactions.remove(tx_id);
        REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);
      }
      OUTCOME_TRY(runtime.commitState(state));
    }

    return std::make_tuple(applied, out, code);
  }

  outcome::result<void> MultisigUtilsImplV2::purgeApprovals(
      State &state, const Address &address) const {
    OUTCOME_TRY(tx_ids, state.pending_transactions.keys());

    for (const auto &tx_id : tx_ids) {
      OUTCOME_TRY(tx, state.pending_transactions.get(tx_id));

      const auto addr_approved =
          std::find(tx.approved.begin(), tx.approved.end(), address);
      if (addr_approved != tx.approved.end()) {
        tx.approved.erase(addr_approved);
      }

      if (tx.approved.empty()) {
        OUTCOME_TRY(state.pending_transactions.remove(tx_id));
      } else {
        OUTCOME_TRY(state.pending_transactions.set(tx_id, tx));
      }
    }
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v2::multisig
