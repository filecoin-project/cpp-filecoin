/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/impl/multisig_utils_impl_v3.hpp"

namespace fc::vm::actor::builtin::v3::multisig {

  outcome::result<ApproveTransactionResult>
  MultisigUtilsImplV3::executeTransaction(
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

      // Starting at version 6 we first check if the transaction exists before
      // deleting. This allows 1 out of n multisig swaps and removes initiated
      // by the swapped/removed signer to go through without an illegal state
      // error

      const auto tx_exists = state.pending_transactions.has(tx_id);
      REQUIRE_NO_ERROR(tx_exists, VMExitCode::kErrIllegalState);

      if (tx_exists.value()) {
        const auto result = state.pending_transactions.remove(tx_id);
        REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);
      }
      OUTCOME_TRY(runtime.commitState(state));
    }

    return std::make_tuple(applied, out, code);
  }
}  // namespace fc::vm::actor::builtin::v3::multisig
