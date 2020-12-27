/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/multisig/multisig_utils.hpp"

namespace fc::vm::actor::builtin::v0::multisig {
  using utils::multisig::ApproveTransactionResult;

  class MultisigUtilsImplV0 : public utils::multisig::MultisigUtils {
   public:
    outcome::result<void> assertCallerIsSignable(
        const Runtime &runtime) const override;

    outcome::result<void> assertCallerIsReceiver(
        const Runtime &runtime) const override;

    outcome::result<void> assertCallerIsSigner(
        const Runtime &runtime, const State &state) const override;

    outcome::result<Address> getResolvedAddress(
        Runtime &runtime, const Address &address) const override;

    BigInt amountLocked(const State &state,
                        const ChainEpoch &elapsed_epoch) const override;

    outcome::result<void> assertAvailable(
        const State &state,
        const TokenAmount &current_balance,
        const TokenAmount &amount_to_spend,
        const ChainEpoch &current_epoch) const override;

    outcome::result<ApproveTransactionResult> approveTransaction(
        Runtime &runtime,
        const TransactionId &tx_id,
        Transaction &transaction) const override;

    outcome::result<ApproveTransactionResult> executeTransaction(
        Runtime &runtime,
        State &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;

    outcome::result<void> purgeApprovals(
        State &state, const Address &address) const override {
      // Not implemented for v0
      return outcome::success();
    }
  };
}  // namespace fc::vm::actor::builtin::v0::multisig
