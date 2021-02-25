/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/payment_channel_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {

  struct PaymentChannelActorState : states::PaymentChannelActorState {
    explicit PaymentChannelActorState()
        : states::PaymentChannelActorState(ActorVersion::kVersion0) {}
  };
  CBOR_TUPLE(PaymentChannelActorState,
             from,
             to,
             to_send,
             settling_at,
             min_settling_height,
             lanes)
}  // namespace fc::vm::actor::builtin::v0::payment_channel

namespace fc {
  template <>
  struct Ipld::Visit<
      vm::actor::builtin::v0::payment_channel::PaymentChannelActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v0::payment_channel::PaymentChannelActorState
            &state,
        const Visitor &visit) {
      visit(state.lanes);
    }
  };
}  // namespace fc
