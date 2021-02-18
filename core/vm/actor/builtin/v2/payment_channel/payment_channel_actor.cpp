/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"

#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::payment_channel {
  using toolchain::Toolchain;

  outcome::result<Address> resolveAccount(Runtime &runtime,
                                          const Address &address,
                                          const CodeId &accountCodeCid) {
    CHANGE_ERROR_A(
        resolved, runtime.resolveOrCreate(address), VMExitCode::kErrNotFound);

    CHANGE_ERROR_A(
        code, runtime.getActorCodeID(resolved), VMExitCode::kErrForbidden);
    if (code != accountCodeCid) {
      return VMExitCode::kErrForbidden;
    }
    return std::move(resolved);
  }

  // Construct
  //============================================================================

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());

    REQUIRE_NO_ERROR_A(
        to,
        resolveAccount(runtime, params.to, address_matcher->getAccountCodeId()),
        VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(
        from,
        resolveAccount(
            runtime, params.from, address_matcher->getAccountCodeId()),
        VMExitCode::kErrIllegalState);

    State state{from, to, 0, 0, 0, {}};
    IpldPtr {
      runtime
    }
    ->load(state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // UpdateChannelState
  //============================================================================

  outcome::result<void> UpdateChannelState::checkPaychannelAddr(
      const Runtime &runtime, const SignedVoucher &voucher) {
    const auto paych_addr = runtime.getCurrentReceiver();
    OUTCOME_TRY(voucher_addr, runtime.resolveAddress(voucher.channel));
    OUTCOME_TRY(runtime.validateArgument(paych_addr == voucher_addr));
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
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(
        std::vector<Address>{state.from, state.to}));
    const auto &voucher = params.signed_voucher;
    OUTCOME_TRY(v0::payment_channel::UpdateChannelState::checkSignature(
        runtime, state, voucher));
    OUTCOME_TRY(checkPaychannelAddr(runtime, voucher));
    OUTCOME_TRY(v0::payment_channel::UpdateChannelState::checkVoucher(
        runtime, params.secret, voucher));
    OUTCOME_TRY(voucherExtra(runtime, voucher));
    OUTCOME_TRYA(
        state,
        runtime.getCurrentActorStateCbor<State>());  // Lotus gas conformance
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
