/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_MULTISIG_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_MULTISIG_ACTOR_STATE_HPP

#include "adt/map.hpp"
#include "adt/uvarint_key.hpp"
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v0::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using TransactionId = int64_t;
  using TransactionKeyer = adt::VarintKeyer;
  using ApproveTransactionResult = std::tuple<bool, Buffer, VMExitCode>;

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
    outcome::result<Buffer> hash(Runtime &runtime) const;
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
  struct State {
    std::vector<Address> signers;
    size_t threshold{};
    TransactionId next_transaction_id{};

    // Linear lock
    TokenAmount initial_balance{};
    ChainEpoch start_epoch{};
    EpochDuration unlock_duration{};

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
     * Check if address is signer and allows to get iterator of this signer
     * @param address - address to check
     * @return iterator and true if address is signer
     */
    inline auto checkSigner(const Address &address) const {
      auto it = std::find(signers.begin(), signers.end(), address);
      const bool result = it != signers.end();
      return std::make_tuple(result, it);
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
        Runtime &runtime,
        const TransactionId &tx_id,
        const Buffer &proposal_hash) const;

    /**
     * Get amount locked for elapsed epoch
     * @param elapsed_epoch - elapsed block number
     * @return - return amount locked
     */
    BigInt amountLocked(const ChainEpoch &elapsed_epoch) const;

    /**
     * Check availability of funds
     * @param current_balance - current balance
     * @param amount_to_spend - amount of money to spend
     * @param current_epoch - current epoch
     * @return nothing or error occurred
     */
    outcome::result<void> assertAvailable(
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const;
  };
  CBOR_TUPLE(State,
             signers,
             threshold,
             next_transaction_id,
             initial_balance,
             start_epoch,
             unlock_duration,
             pending_transactions)

  /**
   * Approve pending transaction and try to execute.
   * @param runtime - execution context
   * @param tx_id - transaction id
   * @param transaction - transaction to approve
   * @return applied flag, result of sending a message and result code of
   * sending a message
   */
  outcome::result<ApproveTransactionResult> approveTransaction(
      Runtime &runtime, const TransactionId &tx_id, Transaction &transaction);

  /**
   * Execute transaction if approved. Send pending transaction if threshold is
   * met.
   * @param runtime - execution context
   * @param state - actor state
   * @param tx_id - transaction id
   * @param transaction - transaction to approve
   * @return applied flag, result of sending a message and result code of
   * sending a message
   */
  outcome::result<ApproveTransactionResult> executeTransaction(
      Runtime &runtime,
      State &state,
      const TransactionId &tx_id,
      const Transaction &transaction);

}  // namespace fc::vm::actor::builtin::v0::multisig

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::multisig::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::multisig::State &state,
                     const Visitor &visit) {
      visit(state.pending_transactions);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_MULTISIG_ACTOR_STATE_HPP
