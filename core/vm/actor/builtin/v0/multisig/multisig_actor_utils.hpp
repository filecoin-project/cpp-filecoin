/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/multisig_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::MultisigActorStatePtr;
  using types::multisig::Transaction;
  using types::multisig::TransactionId;
  using utils::multisig::ApproveTransactionResult;

  class MultisigUtils : public utils::MultisigUtils {
   public:
    explicit MultisigUtils(Runtime &r) : utils::MultisigUtils(r) {}

    outcome::result<void> assertCallerIsSigner(
        const MultisigActorStatePtr &state) const override;

    outcome::result<Address> getResolvedAddress(
        const Address &address) const override;

    BigInt amountLocked(const MultisigActorStatePtr &state,
                        const ChainEpoch &elapsed_epoch) const override;

    outcome::result<void> assertAvailable(
        const MultisigActorStatePtr &state,
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const override;

    outcome::result<ApproveTransactionResult> approveTransaction(
        const TransactionId &tx_id, Transaction &transaction) const override;

    outcome::result<ApproveTransactionResult> executeTransaction(
        MultisigActorStatePtr &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;

    outcome::result<void> purgeApprovals(
        MultisigActorStatePtr &state, const Address &address) const override {
      // Not implemented for v0
      return outcome::success();
    }
  };
}  // namespace fc::vm::actor::builtin::v0::multisig
