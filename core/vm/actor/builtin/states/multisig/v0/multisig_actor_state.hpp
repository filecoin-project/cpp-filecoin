/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/multisig/multisig_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::v0::multisig {

  struct MultisigActorState : states::MultisigActorState {};
  CBOR_TUPLE(MultisigActorState,
             signers,
             threshold,
             next_transaction_id,
             initial_balance,
             start_epoch,
             unlock_duration,
             pending_transactions)

}  // namespace fc::vm::actor::builtin::v0::multisig

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v0::multisig::MultisigActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v0::multisig::MultisigActorState &state,
        const Visitor &visit) {
      visit(state.pending_transactions);
    }
  };
}  // namespace fc::cbor_blake
