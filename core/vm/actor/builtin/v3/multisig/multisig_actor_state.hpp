/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using TransactionId = v2::multisig::TransactionId;

  // Multisig actor state v3 is identical to Multisigl actor state v2
  using v2::multisig::kSignersMax;
  using Transaction = v2::multisig::Transaction;
  using ProposalHashData = v2::multisig::ProposalHashData;
  using State = v2::multisig::State;

}  // namespace fc::vm::actor::builtin::v3::multisig
