/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/account/account_actor.hpp"

#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::account {
  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    if (!params.isKeyType()) {
      return VMExitCode::kAccountActorCreateWrongAddressType;
    }
    OUTCOME_TRY(runtime.commitState(AccountActorState{params}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PubkeyAddress) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<AccountActorState>());
    return state.address;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<PubkeyAddress>(),
  };
}  // namespace fc::vm::actor::builtin::account
