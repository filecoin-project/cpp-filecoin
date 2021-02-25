/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/cron/cron_actor.hpp"

namespace fc::vm::actor::builtin::v0::cron {
  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    auto state =
        runtime.stateManager()->createCronActorState(runtime.getActorVersion());
    state->entries = params;
    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }
  ACTOR_METHOD_IMPL(EpochTick) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(state, runtime.stateManager()->getCronActorState());
    for (const auto &entry : state->entries) {
      OUTCOME_TRY(asExitCode(
          runtime.send(entry.to_addr, entry.method_num, {}, BigInt(0))));
    }
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<EpochTick>(),
  };
}  // namespace fc::vm::actor::builtin::v0::cron
