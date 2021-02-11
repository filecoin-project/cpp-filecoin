/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/multisig/impl/multisig_utils_impl_v0.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using utils::multisig::ApproveTransactionResult;
  using v0::multisig::TransactionId;

  class MultisigUtilsImplV2 : public v0::multisig::MultisigUtilsImplV0 {
   public:
    BigInt amountLocked(const State &state,
                        const ChainEpoch &elapsed_epoch) const override;

    outcome::result<void> assertAvailable(
        const State &state,
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const override;

    outcome::result<ApproveTransactionResult> executeTransaction(
        Runtime &runtime,
        State &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;

    outcome::result<void> purgeApprovals(State &state,
                                         const Address &address) const override;
  };
}  // namespace fc::vm::actor::builtin::v2::multisig
