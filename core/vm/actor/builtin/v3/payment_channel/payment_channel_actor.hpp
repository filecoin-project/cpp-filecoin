/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V3_PAYMENT_CHANNEL_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V3_PAYMENT_CHANNEL_ACTOR_HPP

#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v3/payment_channel/payment_channel_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::payment_channel {

  /**
   * Payment channel actor v3 is almost identical to Payment channel actor v2
   */

  constexpr auto kLaneLimit = v2::payment_channel::kLaneLimit;
  constexpr auto kSettleDelay = v2::payment_channel::kSettleDelay;
  constexpr auto kMaxSecretSize = v2::payment_channel::kMaxSecretSize;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address from;
      Address to;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, from, to)

  using UpdateChannelState = v2::payment_channel::UpdateChannelState;
  using Settle = v2::payment_channel::Settle;
  using Collect = v2::payment_channel::Collect;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::payment_channel

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V3_PAYMENT_CHANNEL_ACTOR_HPP
