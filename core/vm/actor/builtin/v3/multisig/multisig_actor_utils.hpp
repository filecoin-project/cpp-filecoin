/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/multisig/multisig_actor_utils.hpp"
#include "vm/actor/builtin/v3/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using runtime::Runtime;
  using utils::multisig::ApproveTransactionResult;
  using v2::multisig::TransactionId;

  class MultisigUtils : public v2::multisig::MultisigUtils {
   public:
    MultisigUtils(Runtime &r) : v2::multisig::MultisigUtils::MultisigUtils(r) {}

    outcome::result<ApproveTransactionResult> executeTransaction(
        State &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;
  };
}  // namespace fc::vm::actor::builtin::v3::multisig
