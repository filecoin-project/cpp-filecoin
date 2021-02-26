/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/multisig_actor_state.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::states {
  using fc::vm::runtime::Runtime;

  bool multisig::Transaction::operator==(
      const multisig::Transaction &other) const {
    return to == other.to && value == other.value && method == other.method
           && params == other.params && approved == other.approved;
  }

  outcome::result<Buffer> multisig::Transaction::hash(Runtime &runtime) const {
    ProposalHashData hash_data(*this);
    OUTCOME_TRY(to_hash, codec::cbor::encode(hash_data));
    OUTCOME_TRY(hash, runtime.hashBlake2b(to_hash));
    return Buffer{gsl::make_span(hash)};
  }

  void MultisigActorState::setLocked(const ChainEpoch &start_epoch,
                                     const EpochDuration &unlock_duration,
                                     const TokenAmount &lockedAmount) {
    this->start_epoch = start_epoch;
    this->unlock_duration = unlock_duration;
    this->initial_balance = lockedAmount;
  }

  outcome::result<multisig::Transaction>
  MultisigActorState::getPendingTransaction(
      const multisig::TransactionId &tx_id) const {
    OUTCOME_TRY(pending_tx, pending_transactions.tryGet(tx_id));
    if (!pending_tx) {
      ABORT(VMExitCode::kErrNotFound);
    }

    return std::move(pending_tx.get());
  }

  outcome::result<multisig::Transaction> MultisigActorState::getTransaction(
      Runtime &runtime,
      const multisig::TransactionId &tx_id,
      const Buffer &proposal_hash) const {
    OUTCOME_TRY(transaction, getPendingTransaction(tx_id));
    REQUIRE_NO_ERROR_A(
        hash, transaction.hash(runtime), VMExitCode::kErrIllegalState);
    if (!proposal_hash.empty() && (proposal_hash != hash)) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    return std::move(transaction);
  }
}  // namespace fc::vm::actor::builtin::states
