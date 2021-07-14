/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/multisig/multisig_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using runtime::Runtime;
  using states::MultisigActorStatePtr;
  using types::multisig::Transaction;
  using types::multisig::TransactionId;
  using utils::multisig::ApproveTransactionResult;

  class MultisigUtils : public v2::multisig::MultisigUtils {
   public:
    explicit MultisigUtils(Runtime &r)
        : v2::multisig::MultisigUtils::MultisigUtils(r) {}

    outcome::result<ApproveTransactionResult> executeTransaction(
        MultisigActorStatePtr &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;
  };
}  // namespace fc::vm::actor::builtin::v3::multisig
