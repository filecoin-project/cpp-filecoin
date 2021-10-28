/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/multisig/multisig_actor_state.hpp"

#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::states {

  void MultisigActorState::setLocked(const ChainEpoch &start_epoch,
                                     const EpochDuration &unlock_duration,
                                     const TokenAmount &lockedAmount) {
    this->start_epoch = start_epoch;
    this->unlock_duration = unlock_duration;
    this->initial_balance = lockedAmount;
  }

  outcome::result<Transaction> MultisigActorState::getPendingTransaction(
      const TransactionId &tx_id) const {
    OUTCOME_TRY(pending_tx, pending_transactions.tryGet(tx_id));
    if (!pending_tx) {
      ABORT(VMExitCode::kErrNotFound);
    }

    return std::move(pending_tx.get());
  }

  outcome::result<Transaction> MultisigActorState::getTransaction(
      Runtime &runtime,
      const TransactionId &tx_id,
      const Bytes &proposal_hash) const {
    OUTCOME_TRY(transaction, getPendingTransaction(tx_id));
    REQUIRE_NO_ERROR_A(
        hash, transaction.hash(runtime), VMExitCode::kErrIllegalState);
    if (!proposal_hash.empty() && (proposal_hash != hash)) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    return std::move(transaction);
  }
}  // namespace fc::vm::actor::builtin::states
