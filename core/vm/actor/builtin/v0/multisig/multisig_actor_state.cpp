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
}  // namespace fc::vm::actor::builtin::v0::multisig
