/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/multisig/impl/multisig_utils_impl_v2.hpp"
#include "vm/actor/builtin/v3/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using runtime::Runtime;
  using utils::multisig::ApproveTransactionResult;
  using v2::multisig::TransactionId;

  class MultisigUtilsImplV3 : public v2::multisig::MultisigUtilsImplV2 {
   public:
    outcome::result<ApproveTransactionResult> executeTransaction(
        Runtime &runtime,
        State &state,
        const TransactionId &tx_id,
        const Transaction &transaction) const override;
  };
}  // namespace fc::vm::actor::builtin::v3::multisig
