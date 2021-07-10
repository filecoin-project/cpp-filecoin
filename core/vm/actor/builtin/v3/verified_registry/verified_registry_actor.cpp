/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor.hpp"

#include "vm/actor/builtin/states/verified_registry_actor_state.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {
  using states::VerifiedRegistryActorStatePtr;
  using toolchain::Toolchain;

  // UseBytes
  //============================================================================

  ACTOR_METHOD_IMPL(UseBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));

    REQUIRE_NO_ERROR_A(client,
                       runtime.resolveAddress(params.address),
                       VMExitCode::kErrIllegalState);

    const auto utils = Toolchain::createVerifRegUtils(runtime);
    OUTCOME_TRY(utils->checkDealSize(params.deal_size));
    OUTCOME_TRY(state, runtime.getActorState<VerifiedRegistryActorStatePtr>());

    auto clientCapAssert = [&runtime](bool condition) -> outcome::result<void> {
      return runtime.requireState(condition);
    };

    OUTCOME_TRY(v0::verified_registry::UseBytes::useBytes(
        runtime, state, client, params.deal_size, clientCapAssert));
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
}  // namespace fc::vm::actor::builtin::v3::verified_registry
