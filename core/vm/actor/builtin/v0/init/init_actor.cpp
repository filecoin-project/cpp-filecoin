/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/init/init_actor.hpp"

#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::init {
  using states::InitActorStatePtr;
  using toolchain::Toolchain;

  // Construct
  //============================================================================

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    InitActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    state->network_name = params.network_name;
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // Exec
  //============================================================================

  bool Exec::canExec(const Runtime &runtime,
                     const CID &caller_code_id,
                     const CID &exec_code_id) {
    bool result = false;
    const auto matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());

    if (exec_code_id == matcher->getStorageMinerCodeId()) {
      result = caller_code_id == matcher->getStoragePowerCodeId();
    } else if (exec_code_id == matcher->getPaymentChannelCodeId()
               || exec_code_id == matcher->getMultisigCodeId()) {
      result = true;
    }
    return result;
  }

  ACTOR_METHOD_IMPL(Exec) {
    const auto caller_code_id =
        runtime.getActorCodeID(runtime.getImmediateCaller());
    const auto utils = Toolchain::createInitActorUtils(runtime);
    OUTCOME_TRY(utils->check(!caller_code_id.has_error()));
    if (!canExec(runtime, caller_code_id.value(), params.code)) {
      ABORT(VMExitCode::kErrForbidden);
    }

    OUTCOME_TRY(actor_address, runtime.createNewActorAddress());

    OUTCOME_TRY(state, runtime.getActorState<InitActorStatePtr>());
    OUTCOME_TRY(id_address, state->addActor(actor_address));
    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(runtime.createActor(id_address,
                                    Actor{params.code, kEmptyObjectCid, 0, 0}));
    REQUIRE_SUCCESS(runtime.send(id_address,
                                 kConstructorMethodNumber,
                                 params.params,
                                 runtime.getMessage().get().value));

    return Result{id_address, actor_address};
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Exec>(),
  };

}  // namespace fc::vm::actor::builtin::v0::init
