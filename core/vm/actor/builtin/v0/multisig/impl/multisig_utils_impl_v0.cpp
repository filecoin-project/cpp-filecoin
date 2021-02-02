/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/multisig/impl/multisig_utils_impl_v0.hpp"

namespace fc::vm::actor::builtin::v0::multisig {

  outcome::result<void> MultisigUtilsImplV0::assertCallerIsSigner(
      const Runtime &runtime, const State &state) const {
    const auto proposer = runtime.getImmediateCaller();
    if (!state.isSigner(proposer)) {
      ABORT(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }

  outcome::result<Address> MultisigUtilsImplV0::getResolvedAddress(
      Runtime &runtime, const Address &address) const {
    REQUIRE_NO_ERROR_A(resolved,
                       runtime.resolveAddress(address),
                       VMExitCode::kErrIllegalState);
    return std::move(resolved);
  }

  BigInt MultisigUtilsImplV0::amountLocked(
      const State &state, const ChainEpoch &elapsed_epoch) const {
    if (elapsed_epoch >= state.unlock_duration) {
      return 0;
    }
    if (elapsed_epoch < 0) {
      return state.initial_balance;
    }

    const auto unit_locked =
        bigdiv(state.initial_balance, state.unlock_duration);
    return unit_locked * (state.unlock_duration - elapsed_epoch);
  }

  outcome::result<void> MultisigUtilsImplV0::assertAvailable(
      const State &state,
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
        amountLocked(state, current_epoch - state.start_epoch);
    if (remaining_balance < amount_locked) {
      ABORT(VMExitCode::kErrInsufficientFunds);
    }
    return outcome::success();
  }

  outcome::result<ApproveTransactionResult>
  MultisigUtilsImplV0::approveTransaction(Runtime &runtime,
                                          const TransactionId &tx_id,
                                          Transaction &transaction) const {
    const Address caller = runtime.getImmediateCaller();
    if (std::find(
            transaction.approved.begin(), transaction.approved.end(), caller)
        != transaction.approved.end()) {
      ABORT(VMExitCode::kErrForbidden);
    }
    transaction.approved.push_back(caller);

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    REQUIRE_NO_ERROR(state.pending_transactions.set(tx_id, transaction),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));

    return executeTransaction(runtime, state, tx_id, transaction);
  }

  outcome::result<ApproveTransactionResult>
  MultisigUtilsImplV0::executeTransaction(
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
      if (send_result.has_error()) {
        if (!isVMExitCode(send_result.error())) {
          return send_result.error();
        }
        code = static_cast<VMExitCode>(send_result.error().value());
      } else {
        out = send_result.value();
      }
      applied = true;

      // Lotus gas conformance
      OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());

      REQUIRE_NO_ERROR(state.pending_transactions.remove(tx_id),
                       VMExitCode::kErrIllegalState);
      OUTCOME_TRY(runtime.commitState(state));
    }

    return std::make_tuple(applied, out, code);
  }

}  // namespace fc::vm::actor::builtin::v0::multisig
