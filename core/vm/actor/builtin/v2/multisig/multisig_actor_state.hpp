/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::multisig {

  struct MultisigActorState : states::MultisigActorState {
    MultisigActorState()
        : states::MultisigActorState(ActorVersion::kVersion2) {}
  };

  CBOR_TUPLE(MultisigActorState,
             signers,
             threshold,
             next_transaction_id,
             initial_balance,
             start_epoch,
             unlock_duration,
             pending_transactions)

}  // namespace fc::vm::actor::builtin::v2::multisig

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v2::multisig::MultisigActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v2::multisig::MultisigActorState &state,
        const Visitor &visit) {
      visit(state.pending_transactions);
    }
  };
}  // namespace fc
