/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v3/payment_channel/payment_channel_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::payment_channel {

  /**
   * Payment channel actor v3 is almost identical to Payment channel actor v2
   */

  constexpr auto kLaneLimit = v2::payment_channel::kLaneLimit;
  constexpr auto kSettleDelay = v2::payment_channel::kSettleDelay;
  constexpr auto kMaxSecretSize = v2::payment_channel::kMaxSecretSize;

  using Construct = v2::payment_channel::Construct;
  using UpdateChannelState = v2::payment_channel::UpdateChannelState;
  using Settle = v2::payment_channel::Settle;
  using Collect = v2::payment_channel::Collect;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::payment_channel
