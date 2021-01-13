/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v2/codes.hpp"

namespace fc::vm::actor::builtin::v2::payment_channel {
  using crypto::signature::Signature;
  using primitives::TokenAmount;
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

    if (readonly_state.settling_at != 0
        && runtime.getCurrentEpoch() >= readonly_state.settling_at) {
      return VMExitCode::kErrFirstActorSpecificExitCode;
    }
    if (params.secret.size() > kMaxSecretSize) {
      return VMExitCode::kErrIllegalArgument;
    }
    const auto &voucher = params.signed_voucher;
    auto voucher_signable = voucher;
    voucher_signable.signature_bytes = boost::none;
    OUTCOME_TRY(voucher_signable_bytes, codec::cbor::encode(voucher_signable));
    auto &signer = runtime.getImmediateCaller() != readonly_state.to
                       ? readonly_state.to
                       : readonly_state.from;

    const Buffer signature_bytes =
        voucher.signature_bytes ? voucher.signature_bytes.get() : Buffer{};
    OUTCOME_TRY(verified,
                runtime.verifySignatureBytes(
                    signature_bytes, signer, voucher_signable_bytes));
    if (!verified) {
      return VMExitCode::kErrIllegalArgument;
    }
    const auto paych_addr = runtime.getCurrentReceiver();
    OUTCOME_TRY(voucher_addr, runtime.resolveAddress(voucher.channel));
    if (paych_addr != voucher_addr) {
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
    if (!voucher.secret_preimage.empty()) {
      OUTCOME_TRY(hash, runtime.hashBlake2b(params.secret));
      auto secret_preimage_copy = voucher.secret_preimage;
      if (gsl::make_span(hash)
          != gsl::make_span(secret_preimage_copy)) {
        return VMExitCode::kErrIllegalArgument;
      }
    }
    if (voucher.extra) {
      OUTCOME_TRY(runtime.send(voucher.extra->actor,
                               voucher.extra->method,
                               voucher.extra->params,
                               0));
    }

    // To be consistence with Lotus to charge gas
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
    for (const auto &merge : voucher.merges) {
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

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::v2::payment_channel
