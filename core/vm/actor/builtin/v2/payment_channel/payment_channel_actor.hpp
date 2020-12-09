/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_PAYMENT_CHANNEL_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_PAYMENT_CHANNEL_ACTOR_HPP

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::payment_channel {

  /**
   * Payment channel actor v2 is almost identical to Payment channel actor v0
   */

  constexpr auto kLaneLimit = v0::payment_channel::kLaneLimit;
  constexpr auto kSettleDelay = v0::payment_channel::kSettleDelay;
  constexpr auto kMaxSecretSize = v0::payment_channel::kMaxSecretSize;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address from;
      Address to;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, from, to)

  struct UpdateChannelState : ActorMethodBase<2> {
    struct Params {
      SignedVoucher signed_voucher;
      Buffer secret;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(UpdateChannelState::Params, signed_voucher, secret)

  using Settle = v0::payment_channel::Settle;
  using Collect = v0::payment_channel::Collect;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::payment_channel

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_PAYMENT_CHANNEL_ACTOR_HPP
