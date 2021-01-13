/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {
  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));

    const auto id_addr = runtime.resolveAddress(params);
    if (id_addr.has_error()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    State state(id_addr.value());
    IpldPtr {
      runtime
    }
    ->load(state);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddVerifier) {
    if (params.allowance < kMinVerifiedDealSize) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.root_key));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());

    const auto found = state.verified_clients.tryGet(params.address);
    REQUIRE_NO_ERROR(found, VMExitCode::kErrIllegalState);
    if (found.value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    const auto result = state.verifiers.set(params.address, params.allowance);
    REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RemoveVerifier) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.root_key));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());

    const auto result = state.verifiers.remove(params);
    REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddVerifiedClient) {
    if (params.allowance < kMinVerifiedDealSize) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (state.root_key == params.address) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());

    // Validate caller is one of the verifiers.
    const auto verifier = runtime.getImmediateCaller();
    const auto verifier_cap = state.verifiers.tryGet(verifier);
    REQUIRE_NO_ERROR(verifier_cap, VMExitCode::kErrIllegalState);
    if (!verifier_cap.value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrNotFound));
    }

    // Validate client to be added isn't a verifier
    const auto address_cap = state.verifiers.tryGet(params.address);
    REQUIRE_NO_ERROR(address_cap, VMExitCode::kErrIllegalState);
    if (address_cap.value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    // Compute new verifier cap and update.
    if (verifier_cap.value().value() < params.allowance) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    const DataCap new_verifier_cap =
        verifier_cap.value().value() - params.allowance;
    const auto update_result = state.verifiers.set(verifier, new_verifier_cap);
    REQUIRE_NO_ERROR(update_result, VMExitCode::kErrIllegalState);

    const auto client_cap = state.verified_clients.tryGet(params.address);
    REQUIRE_NO_ERROR(client_cap, VMExitCode::kErrIllegalState);
    if (client_cap.value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    const auto result =
        state.verified_clients.set(params.address, params.allowance);
    REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UseBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    if (params.deal_size < kMinVerifiedDealSize) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    const auto client_cap = state.verified_clients.tryGet(params.address);
    REQUIRE_NO_ERROR(client_cap, VMExitCode::kErrIllegalState);
    if (!client_cap.value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrNotFound));
    }

    VM_ASSERT(client_cap.value().value() >= 0);
    if (params.deal_size <= client_cap.value().value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    const DataCap new_client_cap =
        client_cap.value().value() - params.deal_size;

    if (new_client_cap < kMinVerifiedDealSize) {
      const auto result = state.verified_clients.remove(params.address);
      REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);
    } else {
      const auto result =
          state.verified_clients.set(params.address, new_client_cap);
      REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);
    }
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RestoreBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    if (params.deal_size < kMinVerifiedDealSize) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    if (state.root_key == params.address) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());
    const auto verifier_cap = state.verifiers.tryGet(params.address);
    REQUIRE_NO_ERROR(verifier_cap, VMExitCode::kErrIllegalState);
    if (verifier_cap.value()) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalArgument));
    }

    const auto client_cap = state.verified_clients.tryGet(params.address);
    REQUIRE_NO_ERROR(verifier_cap, VMExitCode::kErrIllegalState);
    const DataCap new_client_cap =
        (client_cap.value() ? client_cap.value().value() : 0)
        + params.deal_size;

    const auto result =
        state.verified_clients.set(params.address, new_client_cap);
    REQUIRE_NO_ERROR(result, VMExitCode::kErrIllegalState);

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
}  // namespace fc::vm::actor::builtin::v0::verified_registry
