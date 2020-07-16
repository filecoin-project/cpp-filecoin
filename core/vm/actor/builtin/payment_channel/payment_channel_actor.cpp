/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/payment_channel/payment_channel_actor.hpp"

namespace fc::vm::actor::builtin::payment_channel {
  using primitives::address::Protocol;

  outcome::result<Address> resolveAccount(Runtime &runtime,
                                          const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    OUTCOME_TRY(code, runtime.getActorCodeID(resolved));
    if (code != kAccountCodeCid) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    return std::move(resolved);
  }

  outcome::result<State> assertCallerInChannel(Runtime &runtime) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    if (runtime.getImmediateCaller() != state.from
        && runtime.getImmediateCaller() != state.to) {
      return VMExitCode::kPaymentChannelWrongCaller;
    }
    return std::move(state);
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kInitAddress) {
      return VMExitCode::kPaymentChannelWrongCaller;
    }
    OUTCOME_TRY(to, resolveAccount(runtime, params.to));
    OUTCOME_TRY(from, resolveAccount(runtime, params.from));
    OUTCOME_TRY(runtime.commitState(State{from, to, 0, 0, 0, {}}));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdateChannelState) {
    OUTCOME_TRY(state, assertCallerInChannel(runtime));
    auto &voucher = params.signed_voucher;
    if (!voucher.signature) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    auto voucher_signable = voucher;
    voucher_signable.signature = boost::none;
    OUTCOME_TRY(voucher_signable_bytes, codec::cbor::encode(voucher_signable));
    auto &signer =
        runtime.getImmediateCaller() != state.to ? state.to : state.from;
    OUTCOME_TRY(verified,
                runtime.verifySignature(
                    *voucher.signature, signer, voucher_signable_bytes));
    if (!verified) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    if (runtime.getCurrentEpoch() < voucher.time_lock_min) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    if (voucher.time_lock_max != 0
        && runtime.getCurrentEpoch() > voucher.time_lock_max) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    if (!voucher.secret_preimage.empty()
        && gsl::make_span(runtime.hashBlake2b(params.secret))
               != gsl::make_span(voucher.secret_preimage)) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    if (voucher.extra) {
      OUTCOME_TRY(params2,
                  encodeActorParams(
                      PaymentVerifyParams{voucher.extra->data, params.proof}));
      OUTCOME_TRY(runtime.send(
          voucher.extra->actor, voucher.extra->method, params2, 0));
    }

    auto lane_it = state.findLane(voucher.lane);
    if (lane_it == state.lanes.end() || lane_it->id != voucher.lane) {
      if (state.lanes.size() >= kLaneLimit) {
        return VMExitCode::kPaymentChannelIllegalArgument;
      }
      lane_it = state.lanes.insert(lane_it, LaneState{voucher.lane, 0, 0});
    }
    auto &lane = *lane_it;
    if (lane.nonce > voucher.nonce) {
      return VMExitCode::kPaymentChannelIllegalArgument;
    }
    auto redeem = lane.redeem;
    for (auto &merge : voucher.merges) {
      if (merge.lane == voucher.lane) {
        return VMExitCode::kPaymentChannelIllegalArgument;
      }
      lane_it = state.findLane(merge.lane);
      if (lane_it == state.lanes.end() || lane_it->nonce >= merge.nonce) {
        return VMExitCode::kPaymentChannelIllegalArgument;
      }
      redeem += lane_it->redeem;
      lane_it->nonce = merge.nonce;
    }
    lane.nonce = voucher.nonce;
    state.to_send += voucher.amount - redeem;
    lane.redeem = voucher.amount;
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    if (state.to_send < 0 || state.to_send > balance) {
      return VMExitCode::kPaymentChannelIllegalState;
    }
    if (voucher.min_close_height != 0) {
      if (state.settling_at != 0) {
        state.settling_at =
            std::max(state.settling_at, voucher.min_close_height);
      }
      state.min_settling_height =
          std::max(state.min_settling_height, voucher.min_close_height);
    }

    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(Settle) {
    OUTCOME_TRY(state, assertCallerInChannel(runtime));
    if (state.settling_at != 0) {
      return VMExitCode::kPaymentChannelIllegalState;
    }
    state.settling_at = std::max(state.min_settling_height,
                                 runtime.getCurrentEpoch() + kSettleDelay);
    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(Collect) {
    OUTCOME_TRY(state, assertCallerInChannel(runtime));
    if (state.settling_at == 0
        || runtime.getCurrentEpoch() < state.settling_at) {
      return VMExitCode::kPaymentChannelForbidden;
    }
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    OUTCOME_TRY(runtime.sendFunds(state.from, balance - state.to_send));
    OUTCOME_TRY(runtime.sendFunds(state.to, state.to_send));
    state.to_send = 0;
    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::payment_channel
