/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"

#include "vm/actor/builtin/states/payment_channel/payment_channel_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::payment_channel {
  using states::PaymentChannelActorStatePtr;

  // UpdateChannelState
  //============================================================================

  outcome::result<void> UpdateChannelState::checkPaychannelAddr(
      const Runtime &runtime, const SignedVoucher &voucher) {
    const auto paych_addr = runtime.getCurrentReceiver();
    OUTCOME_TRY(voucher_addr, runtime.resolveAddress(voucher.channel));
    VALIDATE_ARG(paych_addr == voucher_addr);
    return outcome::success();
  }

  outcome::result<void> UpdateChannelState::voucherExtra(
      Runtime &runtime, const SignedVoucher &voucher) {
    if (voucher.extra) {
      OUTCOME_TRY(runtime.send(voucher.extra->actor,
                               voucher.extra->method,
                               voucher.extra->params,
                               0));
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdateChannelState) {
    OUTCOME_TRY(state, runtime.getActorState<PaymentChannelActorStatePtr>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(
        std::vector<Address>{state->from, state->to}));
    const auto &voucher = params.signed_voucher;
    OUTCOME_TRY(v0::payment_channel::UpdateChannelState::checkSignature(
        runtime, state, voucher));
    OUTCOME_TRY(checkPaychannelAddr(runtime, voucher));
    OUTCOME_TRY(v0::payment_channel::UpdateChannelState::checkVoucher(
        runtime, params.secret, voucher));
    OUTCOME_TRY(voucherExtra(runtime, voucher));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<PaymentChannelActorStatePtr>());

    OUTCOME_TRY(v0::payment_channel::UpdateChannelState::calculate(
        runtime, state, voucher));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::v2::payment_channel
