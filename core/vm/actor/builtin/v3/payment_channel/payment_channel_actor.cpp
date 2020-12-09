/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::actor::builtin::v3::payment_channel {
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

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<UpdateChannelState>(),
      exportMethod<Settle>(),
      exportMethod<Collect>(),
  };
}  // namespace fc::vm::actor::builtin::v3::payment_channel
