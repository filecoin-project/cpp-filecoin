/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::ChainEpoch;
  using primitives::TokenAmount;

  struct LaneState {
    /** Total amount for vouchers have been redeemed from the lane */
    TokenAmount redeem{};
    uint64_t nonce{};

    inline bool operator==(const LaneState &other) const {
      return redeem == other.redeem && nonce == other.nonce;
    }
  };
  CBOR_TUPLE(LaneState, redeem, nonce)

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
