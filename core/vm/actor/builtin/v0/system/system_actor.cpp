/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/system/system_actor.hpp"

#include "vm/actor/builtin/states/system_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::system {
  using states::SystemActorStatePtr;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    SystemActorStatePtr state{runtime.getActorVersion()};
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  const ActorExports exports{exportMethod<Construct>()};

}  // namespace fc::vm::actor::builtin::v0::system
