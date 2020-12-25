/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::utils::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using v0::multisig::State;
  using v0::multisig::Transaction;
  using v0::multisig::TransactionId;
  using ApproveTransactionResult = std::tuple<bool, Buffer, VMExitCode>;

  class MultisigUtils {
   public:
    virtual ~MultisigUtils() = default;

    /**
     * Check that caller is signable
     * @param runtime - execution context
     */
    virtual outcome::result<void> assertCallerIsSignable(
        const Runtime &runtime) const = 0;

    /**
     * Check that caller is a current receiver
     * @param runtime - execution context
     */
    virtual outcome::result<void> assertCallerIsReceiver(
        const Runtime &runtime) const = 0;

    /**
     * Check that caller is a signer
     * @param runtime - execution context
     * @param state - actor state
     */
    virtual outcome::result<void> assertCallerIsSigner(
        const Runtime &runtime, const State &state) const = 0;

    /**
     * Resolve address
     * @param runtime - execution context
     * @param address - address to resolve
     * @return resolved address
     */
    virtual outcome::result<Address> getResolvedAddress(
        Runtime &runtime, const Address &address) const = 0;

    /**
     * Get amount locked for elapsed epoch
     * @param state - actor state
     * @param elapsed_epoch - elapsed block number
     * @return - return amount locked
     */
    virtual BigInt amountLocked(const State &state,
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
        const State &state,
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const = 0;

    /**
     * Approve pending transaction and try to execute.
     * @param runtime - execution context
     * @param tx_id - transaction id
     * @param transaction - transaction to approve
     * @return applied flag, result of sending a message and result code of
     * sending a message
     */
    virtual outcome::result<ApproveTransactionResult> approveTransaction(
        Runtime &runtime,
        const TransactionId &tx_id,
        Transaction &transaction) const = 0;

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
    virtual outcome::result<ApproveTransactionResult> executeTransaction(
        Runtime &runtime,
        State &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const = 0;
  };
}  // namespace fc::vm::actor::builtin::utils::multisig
