/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/verified_registry/actor.hpp"

namespace fc::vm::actor::builtin::verified_registry {
  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    IpldPtr ipld{runtime};
    OUTCOME_TRY(runtime.commitState(State{params, {ipld}, {ipld}}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddVerifier) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.root_key));
    OUTCOME_TRY(state.verifiers.set(params.address, params.allowance));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveVerifier) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.root_key));
    OUTCOME_TRY(state.verifiers.remove(params));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddVerifiedClient) {
    VM_ASSERT(params.allowance > kMinVerifiedDealSize);
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    auto verifier{runtime.getImmediateCaller()};
    OUTCOME_TRY(verifier_cap, state.verifiers.get(verifier));
    VM_ASSERT(verifier_cap >= params.allowance);
    verifier_cap -= params.allowance;
    OUTCOME_TRY(state.verifiers.set(verifier, verifier_cap));
    OUTCOME_TRY(has_client, state.verified_clients.has(params.address));
    VM_ASSERT(!has_client);
    OUTCOME_TRY(state.verified_clients.set(params.address, params.allowance));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UseBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    VM_ASSERT(params.deal_size >= kMinVerifiedDealSize);
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(client_cap, state.verified_clients.get(params.address));
    VM_ASSERT(client_cap >= 0);
    VM_ASSERT(params.deal_size <= client_cap);
    client_cap -= params.deal_size;
    if (client_cap < kMinVerifiedDealSize) {
      OUTCOME_TRY(state.verified_clients.remove(params.address));
    } else {
      OUTCOME_TRY(state.verified_clients.set(params.address, client_cap));
    }
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RestoreBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    VM_ASSERT(params.deal_size >= kMinVerifiedDealSize);
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(client_cap, state.verified_clients.tryGet(params.address));
    if (!client_cap) {
      client_cap = 0;
    }
    *client_cap += params.deal_size;
    OUTCOME_TRY(state.verified_clients.set(params.address, *client_cap));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Constructor>(),
      exportMethod<AddVerifier>(),
      exportMethod<RemoveVerifier>(),
      exportMethod<AddVerifiedClient>(),
      exportMethod<UseBytes>(),
      exportMethod<RestoreBytes>(),
  };
}  // namespace fc::vm::actor::builtin::verified_registry
