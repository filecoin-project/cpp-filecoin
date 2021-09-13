/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"

#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  using primitives::TokenAmount;
  using toolchain::Toolchain;
  using types::payment_channel::kLaneLimit;
  using types::payment_channel::kSettleDelay;
  using types::payment_channel::LaneState;
  using types::payment_channel::PaymentVerifyParams;

  // Construct
  //============================================================================

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());

    const auto utils = Toolchain::createPaymentChannelUtils(runtime);

    REQUIRE_NO_ERROR_A(
        to,
        utils->resolveAccount(params.to, address_matcher->getAccountCodeId()),
        VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(
        from,
        utils->resolveAccount(params.from, address_matcher->getAccountCodeId()),
        VMExitCode::kErrIllegalState);

    PaymentChannelActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    state->from = from;
    state->to = to;

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // UpdateChannelState
  //============================================================================

  outcome::result<void> UpdateChannelState::checkSignature(
      Runtime &runtime,
      const PaymentChannelActorStatePtr &state,
      const SignedVoucher &voucher) {
    const auto &signer =
        runtime.getImmediateCaller() != state->to ? state->to : state->from;

    REQUIRE_NO_ERROR_A(voucher_signable_bytes,
                       voucher.signingBytes(),
                       VMExitCode::kErrIllegalArgument);

    const Buffer signature_bytes =
        voucher.signature_bytes ? voucher.signature_bytes.get() : Buffer{};
    REQUIRE_NO_ERROR_A(verified,
                       runtime.verifySignatureBytes(
                           signature_bytes, signer, voucher_signable_bytes),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(runtime.validateArgument(verified));
    return outcome::success();
  }

  outcome::result<void> UpdateChannelState::checkPaychannelAddr(
      const Runtime &runtime, const SignedVoucher &voucher) {
    const auto paych_addr = runtime.getCurrentReceiver();
    OUTCOME_TRY(runtime.validateArgument(paych_addr == voucher.channel));
    return outcome::success();
  }

  outcome::result<void> UpdateChannelState::checkVoucher(
      Runtime &runtime, const Buffer &secret, const SignedVoucher &voucher) {
    OUTCOME_TRY(runtime.validateArgument(runtime.getCurrentEpoch()
                                         >= voucher.time_lock_min));
    OUTCOME_TRY(runtime.validateArgument(voucher.time_lock_max == 0
                                         || runtime.getCurrentEpoch()
                                                <= voucher.time_lock_max));
    OUTCOME_TRY(runtime.validateArgument(voucher.amount.sign() >= 0));

    if (!voucher.secret_preimage.empty()) {
      OUTCOME_TRY(hash, runtime.hashBlake2b(secret));
      auto secret_preimage_copy = voucher.secret_preimage;
      OUTCOME_TRY(runtime.validateArgument(
          gsl::make_span(hash) == gsl::make_span(secret_preimage_copy)));
    }
    return outcome::success();
  }

  outcome::result<void> UpdateChannelState::voucherExtra(
      Runtime &runtime, const Buffer &proof, const SignedVoucher &voucher) {
    if (voucher.extra) {
      OUTCOME_TRY(
          params_extra,
          encodeActorParams(PaymentVerifyParams{voucher.extra->params, proof}));
      REQUIRE_SUCCESS(runtime.send(
          voucher.extra->actor, voucher.extra->method, params_extra, 0));
    }

    return outcome::success();
  }

  outcome::result<void> UpdateChannelState::calculate(
      const Runtime &runtime,
      PaymentChannelActorStatePtr &state,
      const SignedVoucher &voucher) {
    OUTCOME_TRY(lanes_size, state->lanes.size());
    OUTCOME_TRY(runtime.validateArgument((lanes_size <= kLaneLimit)
                                         && (voucher.lane <= kLaneLimit)));

    REQUIRE_NO_ERROR_A(maybe_state_lane,
                       state->lanes.tryGet(voucher.lane),
                       VMExitCode::kErrIllegalState);

    LaneState state_lane = maybe_state_lane.has_value()
                               ? maybe_state_lane.value()
                               : LaneState{{}, {}};

    if (maybe_state_lane.has_value()) {
      OUTCOME_TRY(runtime.validateArgument(state_lane.nonce < voucher.nonce));
    }

    BigInt redeem = 0;
    for (const auto &merge : voucher.merges) {
      OUTCOME_TRY(runtime.validateArgument(merge.lane != voucher.lane));
      OUTCOME_TRY(runtime.validateArgument(merge.lane <= kLaneLimit));

      REQUIRE_NO_ERROR_A(maybe_lane,
                         state->lanes.tryGet(merge.lane),
                         VMExitCode::kErrIllegalState);
      OUTCOME_TRY(runtime.validateArgument(maybe_lane.has_value()));

      auto &lane = maybe_lane.value();

      OUTCOME_TRY(runtime.validateArgument(lane.nonce < merge.nonce));

      redeem += lane.redeem;
      lane.nonce = merge.nonce;
      REQUIRE_NO_ERROR(state->lanes.set(merge.lane, lane),
                       VMExitCode::kErrIllegalState);
    }
    state_lane.nonce = voucher.nonce;
    TokenAmount balance_delta = voucher.amount - (redeem + state_lane.redeem);
    state_lane.redeem = voucher.amount;
    TokenAmount send_balance = state->to_send + balance_delta;

    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    OUTCOME_TRY(runtime.validateArgument((send_balance >= 0)
                                         && (send_balance <= balance)));
    state->to_send = send_balance;

    if (voucher.min_close_height != 0) {
      if (state->settling_at != 0) {
        state->settling_at =
            std::max(state->settling_at, voucher.min_close_height);
      }
      state->min_settling_height =
          std::max(state->min_settling_height, voucher.min_close_height);
    }

    REQUIRE_NO_ERROR(state->lanes.set(voucher.lane, state_lane),
                     VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdateChannelState) {
    OUTCOME_TRY(state, runtime.getActorState<PaymentChannelActorStatePtr>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(
        std::vector<Address>{state->from, state->to}));
    const auto &voucher = params.signed_voucher;
    OUTCOME_TRY(checkSignature(runtime, state, voucher));
    OUTCOME_TRY(checkPaychannelAddr(runtime, voucher));
    OUTCOME_TRY(checkVoucher(runtime, params.secret, voucher));
    OUTCOME_TRY(voucherExtra(runtime, params.proof, voucher));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<PaymentChannelActorStatePtr>());

    OUTCOME_TRY(calculate(runtime, state, voucher));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // Settle
  //============================================================================

  ACTOR_METHOD_IMPL(Settle) {
    OUTCOME_TRY(state, runtime.getActorState<PaymentChannelActorStatePtr>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(
        std::vector<Address>{state->from, state->to}));

    if (state->settling_at != 0) {
      ABORT(VMExitCode::kErrIllegalState);
    }
    state->settling_at = std::max(state->min_settling_height,
                                  runtime.getCurrentEpoch() + kSettleDelay);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // Collect
  //============================================================================

  ACTOR_METHOD_IMPL(Collect) {
    OUTCOME_TRY(state, runtime.getActorState<PaymentChannelActorStatePtr>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(
        std::vector<Address>{state->from, state->to}));

    if (state->settling_at == 0
        || runtime.getCurrentEpoch() < state->settling_at) {
      ABORT(VMExitCode::kErrForbidden);
    }

    REQUIRE_SUCCESS(runtime.sendFunds(state->to, state->to_send));
    OUTCOME_TRY(runtime.deleteActor(state->from));

    return outcome::success();
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::v0::payment_channel
