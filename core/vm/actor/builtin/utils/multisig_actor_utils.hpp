/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/multisig/multisig_actor_state.hpp"
#include "vm/actor/builtin/utils/actor_utils.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::MultisigActorStatePtr;
  using types::multisig::Transaction;
  using types::multisig::TransactionId;

  namespace multisig {
    using ApproveTransactionResult = std::tuple<bool, Bytes, VMExitCode>;
  }

  class MultisigUtils : public ActorUtils {
   public:
    explicit MultisigUtils(Runtime &r) : ActorUtils(r) {}

    /**
     * Check that caller is a signer
     * @param state - actor state
     */
    virtual outcome::result<void> assertCallerIsSigner(
        const MultisigActorStatePtr &state) const = 0;

    /**
     * Resolve address
     * @param address - address to resolve
     * @return resolved address
     */
    virtual outcome::result<Address> getResolvedAddress(
        const Address &address) const = 0;

    /**
     * Get amount locked for elapsed epoch
     * @param state - actor state
     * @param elapsed_epoch - elapsed block number
     * @return - return amount locked
     */
    virtual BigInt amountLocked(const MultisigActorStatePtr &state,
                                const ChainEpoch &elapsed_epoch) const = 0;

    /**
     * Check availability of funds
     * @param state - actor state
     * @param current_balance - current balance
     * @param amount_to_spend - amount of money to spend
     * @param current_epoch - current epoch
     * @return nothing or error occurred
     */
    virtual outcome::result<void> assertAvailable(
        const MultisigActorStatePtr &state,
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const = 0;

    /**
     * Approve pending transaction and try to execute.
     * @param tx_id - transaction id
     * @param transaction - transaction to approve
     * @return applied flag, result of sending a message and result code of
     * sending a message
     */
    virtual outcome::result<multisig::ApproveTransactionResult>
    approveTransaction(const TransactionId &tx_id,
                       Transaction &transaction) const = 0;

    /**
     * Execute transaction if approved. Send pending transaction if threshold is
     * met.
     * @param state - actor state
     * @param tx_id - transaction id
     * @param transaction - transaction to approve
     * @return applied flag, result of sending a message and result code of
     * sending a message
     */
    virtual outcome::result<multisig::ApproveTransactionResult>
    executeTransaction(MultisigActorStatePtr &state,
                       const TransactionId &tx_id,
                       const Transaction &transaction) const = 0;

    /**
     * Iterates all pending transactions and removes an address from each list
     * of approvals, if present. If an approval list becomes empty, the pending
     * transaction is deleted.
     * @param state - actor state
     * @param address - address to purge
     */
    virtual outcome::result<void> purgeApprovals(
        MultisigActorStatePtr &state, const Address &address) const = 0;
  };

  using MultisigUtilsPtr = std::shared_ptr<MultisigUtils>;

}  // namespace fc::vm::actor::builtin::utils
