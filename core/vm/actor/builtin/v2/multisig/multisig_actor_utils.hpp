/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/multisig/multisig_actor_utils.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::multisig::Transaction;
  using states::multisig::TransactionId;
  using utils::multisig::ApproveTransactionResult;

  class MultisigUtils : public v0::multisig::MultisigUtils {
   public:
    explicit MultisigUtils(Runtime &r) : v0::multisig::MultisigUtils(r) {}

    BigInt amountLocked(const states::MultisigActorStatePtr &state,
                        const ChainEpoch &elapsed_epoch) const override;

    outcome::result<void> assertAvailable(
        const states::MultisigActorStatePtr &state,
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const override;

    outcome::result<ApproveTransactionResult> executeTransaction(
        states::MultisigActorStatePtr state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;

    outcome::result<void> purgeApprovals(states::MultisigActorStatePtr state,
                                         const Address &address) const override;
  };
}  // namespace fc::vm::actor::builtin::v2::multisig
