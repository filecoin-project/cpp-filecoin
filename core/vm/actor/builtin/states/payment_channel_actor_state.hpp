/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

#include "adt/array.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using types::payment_channel::LaneState;

  struct PaymentChannelActorState : State {
    explicit PaymentChannelActorState(ActorVersion version)
        : State(ActorType::kPaymentChannel, version) {}

    Address from;
    Address to;
    /** Token amount to send on collect after voucher was redeemed */
    TokenAmount to_send{};
    ChainEpoch settling_at{};
    ChainEpoch min_settling_height{};
    adt::Array<LaneState> lanes;
  };

  using PaymentChannelActorStatePtr = std::shared_ptr<PaymentChannelActorState>;

}  // namespace fc::vm::actor::builtin::states
