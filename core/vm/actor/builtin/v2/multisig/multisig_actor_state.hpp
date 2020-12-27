/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_MULTISIG_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_MULTISIG_ACTOR_STATE_HPP

#include "vm/actor/builtin/v0/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::multisig {
  using TransactionId = v0::multisig::TransactionId;

  constexpr int kSignersMax = 256;

  // Multisig actor state v2 is identical to Multisigl actor state v0
  using Transaction = v0::multisig::Transaction;
  using ProposalHashData = v0::multisig::ProposalHashData;
  using State = v0::multisig::State;

}  // namespace fc::vm::actor::builtin::v2::multisig

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_MULTISIG_ACTOR_STATE_HPP
