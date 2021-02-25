/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"

namespace fc::vm::actor::builtin::v2::payment_channel {
  using v0::payment_channel::SignedVoucher;

  /**
   * Payment channel actor v2 is almost identical to Payment channel actor v0
   */

  constexpr auto kLaneLimit = v0::payment_channel::kLaneLimit;
  constexpr auto kSettleDelay = v0::payment_channel::kSettleDelay;
  constexpr auto kMaxSecretSize = v0::payment_channel::kMaxSecretSize;

  using Construct = v0::payment_channel::Construct;

  struct UpdateChannelState : ActorMethodBase<2> {
    struct Params {
      SignedVoucher signed_voucher;
      Buffer secret;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> checkPaychannelAddr(
        const Runtime &runtime, const SignedVoucher &voucher);
    static outcome::result<void> voucherExtra(Runtime &runtime,
                                              const SignedVoucher &voucher);
  };
  CBOR_TUPLE(UpdateChannelState::Params, signed_voucher, secret)

  using Settle = v0::payment_channel::Settle;
  using Collect = v0::payment_channel::Collect;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::payment_channel
