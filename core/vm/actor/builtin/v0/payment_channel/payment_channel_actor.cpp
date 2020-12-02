/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  using primitives::address::Protocol;

  outcome::result<Address> resolveAccount(Runtime &runtime,
                                          const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    OUTCOME_TRY(code, runtime.getActorCodeID(resolved));
    if (code != kAccountCodeCid) {
      return VMExitCode::kErrForbidden;
    }
    return std::move(resolved);
  }

  outcome::result<State> assertCallerInChannel(Runtime &runtime) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    if (runtime.getImmediateCaller() != state.from
        && runtime.getImmediateCaller() != state.to) {
      return VMExitCode::kErrForbidden;
    }
    return std::move(state);
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kInitAddress) {
      return VMExitCode::kSysErrForbidden;
    }
    OUTCOME_TRY(to, resolveAccount(runtime, params.to));
    OUTCOME_TRY(from, resolveAccount(runtime, params.from));

    State state{from, to, 0, 0, 0, {}};
    IpldPtr {
      runtime
    }
    ->load(state);
    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdateChannelState) {
    OUTCOME_TRY(readonly_state, assertCallerInChannel(runtime));
    auto &voucher = params.signed_voucher;
    if (!voucher.signature) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (readonly_state.settling_at != 0
        && runtime.getCurrentEpoch() >= readonly_state.settling_at) {
      return VMExitCode::kErrFirstActorSpecificExitCode;
    }
    if (params.secret.size() > kMaxSecretSize) {
      return VMExitCode::kErrIllegalArgument;
    }
    auto voucher_signable = voucher;
    voucher_signable.signature = boost::none;
    OUTCOME_TRY(voucher_signable_bytes, codec::cbor::encode(voucher_signable));
    auto &signer = runtime.getImmediateCaller() != readonly_state.to
                       ? readonly_state.to
                       : readonly_state.from;
    OUTCOME_TRY(verified,
                runtime.verifySignature(
                    *voucher.signature, signer, voucher_signable_bytes));
    if (!verified) {
      return VMExitCode::kErrIllegalArgument;
    }
    const auto paych_addr = runtime.getCurrentReceiver();
    if (paych_addr != voucher.channel) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (runtime.getCurrentEpoch() < voucher.time_lock_min) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (voucher.time_lock_max != 0
        && runtime.getCurrentEpoch() > voucher.time_lock_max) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (voucher.amount.sign() < 0) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (!voucher.secret_preimage.empty()
        && gsl::make_span(runtime.hashBlake2b(params.secret))
               != gsl::make_span(voucher.secret_preimage)) {
      return VMExitCode::kErrIllegalArgument;
    }
    if (voucher.extra) {
      OUTCOME_TRY(
          params2,
          encodeActorParams(PaymentVerifyParams{voucher.extra->params}));
      OUTCOME_TRY(runtime.send(
          voucher.extra->actor, voucher.extra->method, params2, 0));
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    OUTCOME_TRY(lanes_size, state.lanes.size());
    if (lanes_size > kLaneLimit || voucher.lane > kLaneLimit) {
      return VMExitCode::kErrIllegalArgument;
    }
    OUTCOME_TRY(state_lane, state.lanes.tryGet(voucher.lane));
    if (state_lane) {
      if (state_lane->nonce >= voucher.nonce) {
        return VMExitCode::kErrIllegalArgument;
      }
    } else {
      state_lane = LaneState{{}, {}};
    }
    BigInt redeem = 0;
    for (auto &merge : voucher.merges) {
      if (merge.lane == voucher.lane) {
        return VMExitCode::kErrIllegalArgument;
      }
      if (merge.lane > kLaneLimit) {
        return VMExitCode::kErrIllegalArgument;
      }
      OUTCOME_TRY(lane, state.lanes.tryGet(merge.lane));
      if (!lane || lane->nonce >= merge.nonce) {
        return VMExitCode::kErrIllegalArgument;
      }
      redeem += lane->redeem;
      lane->nonce = merge.nonce;
      OUTCOME_TRY(state.lanes.set(merge.lane, *lane));
    }
    state_lane->nonce = voucher.nonce;
    TokenAmount balance_delta = voucher.amount - (redeem + state_lane->redeem);
    state_lane->redeem = voucher.amount;
    TokenAmount send_balance = state.to_send + balance_delta;

    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    if (send_balance < 0 || send_balance > balance) {
      return VMExitCode::kErrIllegalArgument;
    }
    state.to_send = send_balance;

    if (voucher.min_close_height != 0) {
      if (state.settling_at != 0) {
        state.settling_at =
            std::max(state.settling_at, voucher.min_close_height);
      }
      state.min_settling_height =
          std::max(state.min_settling_height, voucher.min_close_height);
    }

    OUTCOME_TRY(state.lanes.set(voucher.lane, *state_lane));
    OUTCOME_TRY(runtime.commitState(state));
    return fc::outcome::success();
  }

  ACTOR_METHOD_IMPL(Settle) {
    OUTCOME_TRY(state, assertCallerInChannel(runtime));
    if (state.settling_at != 0) {
      return VMExitCode::kErrIllegalState;
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
      return VMExitCode::kErrForbidden;
    }

    OUTCOME_TRY(runtime.sendFunds(state.to, state.to_send));
    OUTCOME_TRY(runtime.deleteActor(state.from));

    return fc::outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::v0::payment_channel
