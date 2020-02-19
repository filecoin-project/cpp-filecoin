/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

using fc::vm::actor::ActorExports;
using fc::vm::actor::kConstructorMethodNumber;
using fc::vm::actor::builtin::storage_power::StoragePowerActorMethods;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::Runtime;

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::construct(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (runtime.getImmediateCaller() != kSystemActorAddress)
    return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(empty_state, StoragePowerActor::createEmptyState(datastore));

  // commit state
  OUTCOME_TRY(state_cid, datastore->setCbor(empty_state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

const ActorExports fc::vm::actor::builtin::storage_power::exports = {
    {kConstructorMethodNumber,
     ActorMethod(StoragePowerActorMethods::construct)}};
