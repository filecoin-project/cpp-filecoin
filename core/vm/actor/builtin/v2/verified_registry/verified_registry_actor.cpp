/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor.hpp"

#include "vm/actor/builtin/states/verified_registry_actor_state.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::verified_registry {
  using states::VerifiedRegistryActorStatePtr;
  using toolchain::Toolchain;

  // AddVerifier
  //============================================================================

  ACTOR_METHOD_IMPL(AddVerifier) {
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.allowance));

    REQUIRE_NO_ERROR_A(verifier,
                       runtime.resolveOrCreate(params.address),
                       VMExitCode::kErrIllegalState);

    OUTCOME_TRY(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state->root_key));
    OUTCOME_TRY(utils->checkAddress(state, verifier));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());

    OUTCOME_TRY(v0::verified_registry::AddVerifier::addVerifier(
        runtime, state, verifier, params.allowance));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // RemoveVerifier
  //============================================================================

  ACTOR_METHOD_IMPL(RemoveVerifier) {
    REQUIRE_NO_ERROR_A(verifier,
                       runtime.resolveOrCreate(params),
                       VMExitCode::kErrIllegalState);

    OUTCOME_TRY(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state->root_key));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());

    REQUIRE_NO_ERROR(state->verifiers.remove(verifier),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // AddVerifiedClient
  //============================================================================

  ACTOR_METHOD_IMPL(AddVerifiedClient) {
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.allowance));

    REQUIRE_NO_ERROR_A(client,
                       runtime.resolveOrCreate(params.address),
                       VMExitCode::kErrIllegalState);

    OUTCOME_TRY(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());
    OUTCOME_TRY(utils->checkAddress(state, client));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());

    OUTCOME_TRY(v0::verified_registry::AddVerifiedClient::addClient(
        runtime, state, client, params.allowance));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // UseBytes
  //============================================================================

  ACTOR_METHOD_IMPL(UseBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));

    REQUIRE_NO_ERROR_A(client,
                       runtime.resolveOrCreate(params.address),
                       VMExitCode::kErrIllegalState);

    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.deal_size));
    OUTCOME_TRY(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());

    auto clientCapAssert = [&runtime](bool condition) -> outcome::result<void> {
      return runtime.vm_assert(condition);
    };
    OUTCOME_TRY(v0::verified_registry::UseBytes::useBytes(
        runtime, state, client, params.deal_size, clientCapAssert));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // RestoreBytes
  //============================================================================

  ACTOR_METHOD_IMPL(RestoreBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));
    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.deal_size));

    REQUIRE_NO_ERROR_A(client,
                       runtime.resolveOrCreate(params.address),
                       VMExitCode::kErrIllegalState);

    OUTCOME_TRY(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());
    OUTCOME_TRY(utils->checkAddress(state, client));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());

    OUTCOME_TRY(v0::verified_registry::RestoreBytes::restoreBytes(
        runtime, state, client, params.deal_size));
    OUTCOME_TRY(runtime.commitState(state));
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
}  // namespace fc::vm::actor::builtin::v2::verified_registry
