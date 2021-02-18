/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/account/account_actor.hpp"

#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::v0::account {
  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    if (!params.isKeyType()) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    auto state = runtime.stateManager()->createAccountActorState(
        runtime.getActorVersion());
    state->address = params;

    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PubkeyAddress) {
    OUTCOME_TRY(state, runtime.stateManager()->getAccountActorState());
    return state->address;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<PubkeyAddress>(),
  };
}  // namespace fc::vm::actor::builtin::v0::account
