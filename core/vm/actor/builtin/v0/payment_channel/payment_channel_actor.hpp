/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/states/payment_channel/payment_channel_actor_state.hpp"
#include "vm/actor/builtin/types/payment_channel/policy.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  using primitives::EpochDuration;
  using primitives::address::Address;
  using states::PaymentChannelActorStatePtr;
  using types::payment_channel::SignedVoucher;

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
      Bytes secret;
      Bytes proof;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> checkSignature(
        Runtime &runtime,
        const PaymentChannelActorStatePtr &state,
        const SignedVoucher &voucher);
    static outcome::result<void> checkPaychannelAddr(
        const Runtime &runtime, const SignedVoucher &voucher);
    static outcome::result<void> checkVoucher(Runtime &runtime,
                                              const Bytes &secret,
                                              const SignedVoucher &voucher);
    static outcome::result<void> voucherExtra(Runtime &runtime,
                                              const Bytes &proof,
                                              const SignedVoucher &voucher);
    static outcome::result<void> calculate(const Runtime &runtime,
                                           PaymentChannelActorStatePtr &state,
                                           const SignedVoucher &voucher);
  };
  CBOR_TUPLE(UpdateChannelState::Params, signed_voucher, secret, proof)

  struct Settle : ActorMethodBase<3> {
    ACTOR_METHOD_DECL();
  };

  struct Collect : ActorMethodBase<4> {
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::payment_channel
