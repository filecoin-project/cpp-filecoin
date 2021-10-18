/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/multisig_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::multisig {

  outcome::result<ApproveTransactionResult> MultisigUtils::executeTransaction(
      MultisigActorStatePtr &state,
      const TransactionId &tx_id,
      const Transaction &transaction) const {
    bool applied = false;
    Bytes out{};
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

      // Starting at version 6 we first check if the transaction exists before
      // deleting. This allows 1 out of n multisig swaps and removes initiated
      // by the swapped/removed signer to go through without an illegal state
      // error

      REQUIRE_NO_ERROR_A(tx_exists,
                         state->pending_transactions.has(tx_id),
                         VMExitCode::kErrIllegalState);

      if (tx_exists) {
        REQUIRE_NO_ERROR(state->pending_transactions.remove(tx_id),
                         VMExitCode::kErrIllegalState);
      }
      OUTCOME_TRY(getRuntime().commitState(state));
    }

    return std::make_tuple(applied, out, code);
  }
}  // namespace fc::vm::actor::builtin::v3::multisig
