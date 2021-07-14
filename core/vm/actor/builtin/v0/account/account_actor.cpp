/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/account/account_actor.hpp"

#include "vm/actor/builtin/states/account_actor_state.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::v0::account {
  using states::AccountActorStatePtr;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    if (!params.isKeyType()) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }

    AccountActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    state->address = params;

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PubkeyAddress) {
    OUTCOME_TRY(state, runtime.getActorState<AccountActorStatePtr>());
    return state->address;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<PubkeyAddress>(),
  };
}  // namespace fc::vm::actor::builtin::v0::account
