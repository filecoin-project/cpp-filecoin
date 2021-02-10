/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::actor::builtin::v3::payment_channel {
  using primitives::TokenAmount;
  using primitives::address::Protocol;
  using v0::payment_channel::resolveAccount;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    REQUIRE_NO_ERROR_A(to,
                       resolveAccount(runtime, params.to, kAccountCodeId),
                       VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(from,
                       resolveAccount(runtime, params.from, kAccountCodeId),
                       VMExitCode::kErrIllegalState);

    State state{from, to, 0, 0, 0, {}};
    IpldPtr {
      runtime
    }
    ->load(state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::v3::payment_channel
