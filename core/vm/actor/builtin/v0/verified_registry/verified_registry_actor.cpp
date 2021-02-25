/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"
#include "vm/actor/builtin/v0/verified_registry/policy.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {
  using toolchain::Toolchain;

  // Construct
  //============================================================================

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));

    const auto id_addr = runtime.resolveAddress(params);

    OUTCOME_TRY(runtime.validateArgument(!id_addr.has_error()));

    auto state = runtime.stateManager()->createVerifiedRegistryActorState(
        runtime.getActorVersion());
    state->root_key = id_addr.value();

    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  // AddVerifier
  //============================================================================

  outcome::result<void> AddVerifier::addVerifier(
      const Runtime &runtime,
      states::VerifiedRegistryActorState &state,
      const Address &verifier,
      const DataCap &allowance) {
    REQUIRE_NO_ERROR_A(found,
                       state.verified_clients.tryGet(verifier),
                       VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.validateArgument(!found.has_value()));

    REQUIRE_NO_ERROR(state.verifiers.set(verifier, allowance),
                     VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddVerifier) {
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.allowance));
    OUTCOME_TRY(state, runtime.stateManager()->getVerifiedRegistryActorState());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state->root_key));
    OUTCOME_TRY(utils->checkAddress(*state, params.address));
    OUTCOME_TRYA(
        state,
        runtime.stateManager()
            ->getVerifiedRegistryActorState());  // Lotus gas conformance
    OUTCOME_TRY(addVerifier(runtime, *state, params.address, params.allowance));
    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  // RemoveVerifier
  //============================================================================

  ACTOR_METHOD_IMPL(RemoveVerifier) {
    OUTCOME_TRY(state, runtime.stateManager()->getVerifiedRegistryActorState());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state->root_key));
    OUTCOME_TRYA(
        state,
        runtime.stateManager()
            ->getVerifiedRegistryActorState());  // Lotus gas conformance

    REQUIRE_NO_ERROR(state->verifiers.remove(params),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  // AddVerifiedClient
  //============================================================================

  outcome::result<void> AddVerifiedClient::addClient(
      const Runtime &runtime,
      states::VerifiedRegistryActorState &state,
      const Address &client,
      const DataCap &allowance) {
    // Validate caller is one of the verifiers.
    const auto verifier = runtime.getImmediateCaller();
    REQUIRE_NO_ERROR_A(maybe_verifier_cap,
                       state.verifiers.tryGet(verifier),
                       VMExitCode::kErrIllegalState);
    if (!maybe_verifier_cap.has_value()) {
      ABORT(VMExitCode::kErrNotFound);
    }

    const auto &verifier_cap = maybe_verifier_cap.value();

    // Validate client to be added isn't a verifier
    REQUIRE_NO_ERROR_A(maybe_address_cap,
                       state.verifiers.tryGet(client),
                       VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.validateArgument(!maybe_address_cap.has_value()));

    // Compute new verifier cap and update.
    OUTCOME_TRY(runtime.validateArgument(verifier_cap >= allowance));

    const DataCap new_verifier_cap = verifier_cap - allowance;
    REQUIRE_NO_ERROR(state.verifiers.set(verifier, new_verifier_cap),
                     VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(maybe_client_cap,
                       state.verified_clients.tryGet(client),
                       VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.validateArgument(!maybe_client_cap.has_value()));

    REQUIRE_NO_ERROR(state.verified_clients.set(client, allowance),
                     VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddVerifiedClient) {
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.allowance));
    OUTCOME_TRY(state, runtime.stateManager()->getVerifiedRegistryActorState());
    OUTCOME_TRY(utils->checkAddress(*state, params.address));
    OUTCOME_TRYA(
        state,
        runtime.stateManager()
            ->getVerifiedRegistryActorState());  // Lotus gas conformance
    OUTCOME_TRY(addClient(runtime, *state, params.address, params.allowance));
    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  // UseBytes
  //============================================================================

  outcome::result<void> UseBytes::useBytes(
      const Runtime &runtime,
      states::VerifiedRegistryActorState &state,
      const Address &client,
      const StoragePower &deal_size,
      CapAssert cap_assert) {
    REQUIRE_NO_ERROR_A(maybe_client_cap,
                       state.verified_clients.tryGet(client),
                       VMExitCode::kErrIllegalState);
    if (!maybe_client_cap.has_value()) {
      ABORT(VMExitCode::kErrNotFound);
    }

    const auto &client_cap = maybe_client_cap.value();

    OUTCOME_TRY(cap_assert(client_cap >= 0));

    OUTCOME_TRY(runtime.validateArgument(deal_size <= client_cap));

    const DataCap new_client_cap = client_cap - deal_size;

    if (new_client_cap < kMinVerifiedDealSize) {
      REQUIRE_NO_ERROR(state.verified_clients.remove(client),
                       VMExitCode::kErrIllegalState);
    } else {
      REQUIRE_NO_ERROR(state.verified_clients.set(client, new_client_cap),
                       VMExitCode::kErrIllegalState);
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UseBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.deal_size));
    OUTCOME_TRY(state, runtime.stateManager()->getVerifiedRegistryActorState());

    auto clientCapAssert = [&runtime](bool condition) -> outcome::result<void> {
      return runtime.vm_assert(condition);
    };
    OUTCOME_TRY(useBytes(
        runtime, *state, params.address, params.deal_size, clientCapAssert));
    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  // RestoreBytes
  //============================================================================

  outcome::result<void> RestoreBytes::restoreBytes(
      const Runtime &runtime,
      states::VerifiedRegistryActorState &state,
      const Address &client,
      const StoragePower &deal_size) {
    REQUIRE_NO_ERROR_A(maybe_verifier_cap,
                       state.verifiers.tryGet(client),
                       VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.validateArgument(!maybe_verifier_cap.has_value()));

    REQUIRE_NO_ERROR_A(maybe_client_cap,
                       state.verified_clients.tryGet(client),
                       VMExitCode::kErrIllegalState);
    const DataCap new_client_cap =
        (maybe_client_cap.has_value() ? maybe_client_cap.value() : 0)
        + deal_size;

    REQUIRE_NO_ERROR(state.verified_clients.set(client, new_client_cap),
                     VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(RestoreBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.deal_size));
    OUTCOME_TRY(state, runtime.stateManager()->getVerifiedRegistryActorState());
    OUTCOME_TRY(utils->checkAddress(*state, params.address));
    OUTCOME_TRYA(
        state,
        runtime.stateManager()
            ->getVerifiedRegistryActorState());  // Lotus gas conformance
    OUTCOME_TRY(
        restoreBytes(runtime, *state, params.address, params.deal_size));
    OUTCOME_TRY(runtime.stateManager()->commitState(state));
    return outcome::success();
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<AddVerifier>(),
      exportMethod<RemoveVerifier>(),
      exportMethod<AddVerifiedClient>(),
      exportMethod<UseBytes>(),
      exportMethod<RestoreBytes>(),
  };
}  // namespace fc::vm::actor::builtin::v0::verified_registry
