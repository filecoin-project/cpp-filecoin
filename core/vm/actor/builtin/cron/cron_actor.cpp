/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/cron/cron_actor.hpp"

namespace fc::vm::actor::builtin::cron {
  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.commitState(State{params}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(EpochTick) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    for (auto &entry : state.entries) {
      OUTCOME_TRY(runtime.send(entry.to_addr, entry.method_num, {}, BigInt(0)));
    }
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<EpochTick>(),
  };
}  // namespace fc::vm::actor::builtin::cron
