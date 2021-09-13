/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/payment_channel/payment_channel_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {

  struct PaymentChannelActorState : states::PaymentChannelActorState {};
  CBOR_TUPLE(PaymentChannelActorState,
             from,
             to,
             to_send,
             settling_at,
             min_settling_height,
             lanes)
}  // namespace fc::vm::actor::builtin::v0::payment_channel

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<
      vm::actor::builtin::v0::payment_channel::PaymentChannelActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v0::payment_channel::PaymentChannelActorState
            &state,
        const Visitor &visit) {
      visit(state.lanes);
    }
  };
}  // namespace fc::cbor_blake
