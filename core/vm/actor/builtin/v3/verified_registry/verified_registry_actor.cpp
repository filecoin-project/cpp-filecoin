/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {
  namespace Utils = utils::verified_registry;

  // UseBytes
  //============================================================================

  ACTOR_METHOD_IMPL(UseBytes) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStorageMarketAddress));

    const auto client = runtime.resolveAddress(params.address);
    REQUIRE_NO_ERROR(client, VMExitCode::kErrIllegalState);

    OUTCOME_TRY(Utils::checkDealSize(params.deal_size));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    auto clientCapAssert = [](bool condition) -> outcome::result<void> {
      if (!condition) {
        ABORT(VMExitCode::kErrIllegalState);
      }
      return outcome::success();
    };
    OUTCOME_TRY(v0::verified_registry::UseBytes::useBytes(
        runtime, state, client.value(), params.deal_size, clientCapAssert));
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
