/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/map.hpp"
#include "adt/uvarint_key.hpp"
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::states {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using TransactionId = int64_t;
  using TransactionKeyer = adt::VarintKeyer;

  /**
   * Multisignaure pending transaction
   */
  struct Transaction {
    Address to;
    TokenAmount value{};
    MethodNumber method{};
    MethodParams params;

    /**
     * @brief List of addresses that approved transaction
     * This address at index 0 is the transaction proposer, order of this slice
     * must be preserved
     */
    std::vector<Address> approved;

    bool operator==(const Transaction &other) const;
    outcome::result<Buffer> hash(fc::vm::runtime::Runtime &runtime) const;
  };
  CBOR_TUPLE(Transaction, to, value, method, params, approved)

  /**
   * Data for a BLAKE2B-256 to be attached to methods referencing proposals via
   * TXIDs. Ensures the existence of a cryptographic reference to the original
   * proposal. Useful for offline signers and for protection when reorgs change
   * a multisig TXID.
   */
  struct ProposalHashData {
    Address requester;
    Address to;
    TokenAmount value{};
    MethodNumber method{};
    MethodParams params;

    ProposalHashData(const Transaction &transaction)
        : requester(!transaction.approved.empty() ? transaction.approved[0]
                                                  : Address{}),
          to(transaction.to),
          value(transaction.value),
          method(transaction.method),
          params(transaction.params) {}
  };
  CBOR_TUPLE(ProposalHashData, requester, to, value, method, params)

  /**
   * State of Multisig Actor instance
   */
  struct MultisigActorState : State {
    explicit MultisigActorState(ActorVersion version)
        : State(ActorType::kMultisig, version) {}

    std::vector<Address> signers;
    size_t threshold{0};
    TransactionId next_transaction_id{0};

    // Linear lock
    TokenAmount initial_balance{0};
    ChainEpoch start_epoch{0};
    EpochDuration unlock_duration{0};

    // List of pending transactions
    adt::Map<Transaction, TransactionKeyer> pending_transactions;

    void setLocked(const ChainEpoch &start_epoch,
                   const EpochDuration &unlock_duration,
                   const TokenAmount &lockedAmount);

    /**
     * Check if address is signer
     * @param address - address to check
     * @return true if address is signer
     */
    inline bool isSigner(const Address &address) const {
      return std::find(signers.begin(), signers.end(), address)
             != signers.end();
    }

    /**
     * Get pending transaction
     * @param tx_id - transaction id
     * @return transaction
     */
    outcome::result<Transaction> getPendingTransaction(
        const TransactionId &tx_id) const;

    /**
     * Get transaction from the state tree
     * @param tx_id - transaction id
     * @param proposal_hash - expected hash of transaction
     * @return transaction
     */
    outcome::result<Transaction> getTransaction(
        fc::vm::runtime::Runtime &runtime,
        const TransactionId &tx_id,
        const Buffer &proposal_hash) const;
  };

  using MultisigActorStatePtr = std::shared_ptr<MultisigActorState>;

}  // namespace fc::vm::actor::builtin::states