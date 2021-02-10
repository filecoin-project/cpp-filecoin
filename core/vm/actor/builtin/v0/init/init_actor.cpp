/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/init/init_actor.hpp"

#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::actor::builtin::v0::init {
  bool canExec(const CID &caller_code_id, const CID &exec_code_id) {
    bool result = false;
    if (exec_code_id == kStorageMinerCodeId) {
      result = caller_code_id == kStoragePowerCodeId;
    } else if (exec_code_id == kPaymentChannelCodeCid
               || exec_code_id == kMultisigCodeId) {
      result = true;
    }
    return result;
  }

  outcome::result<Address> InitActorState::addActor(const Address &address) {
    const auto id = next_id;
    OUTCOME_TRY(address_map.set(address, id));
    ++next_id;
    return Address::makeFromId(id);
  }

  // Construct
  //============================================================================

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    InitActorState state;
    runtime.getIpfsDatastore()->load(state);
    state.network_name = params.network_name;
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  // Exec
  //============================================================================

  outcome::result<void> Exec::checkCaller(const Runtime &runtime,
                                          const CodeId &code,
                                          CallerAssert caller_assert,
                                          ExecAssert exec_assert) {
    const auto caller_code_id =
        runtime.getActorCodeID(runtime.getImmediateCaller());
    OUTCOME_TRY(caller_assert(!caller_code_id.has_error()));
    if (!exec_assert(caller_code_id.value(), code)) {
      ABORT(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }

  outcome::result<void> Exec::createActor(Runtime &runtime,
                                          const Address &id_address,
                                          const Exec::Params &params) {
    OUTCOME_TRY(runtime.createActor(id_address,
                                    Actor{params.code, kEmptyObjectCid, 0, 0}));
    REQUIRE_SUCCESS(runtime.send(id_address,
                                 kConstructorMethodNumber,
                                 params.params,
                                 runtime.getMessage().get().value));
    return outcome::success();
  }

  outcome::result<Exec::Result> Exec::execute(Runtime &runtime,
                                              const Exec::Params &params,
                                              CallerAssert caller_assert,
                                              ExecAssert exec_assert) {
    OUTCOME_TRY(checkCaller(runtime, params.code, caller_assert, exec_assert));

    OUTCOME_TRY(actor_address, runtime.createNewActorAddress());

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<InitActorState>());
    OUTCOME_TRY(id_address, state.addActor(actor_address));
    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(createActor(runtime, id_address, params));

    return Result{id_address, actor_address};
  }

  ACTOR_METHOD_IMPL(Exec) {
    auto caller_assert = [&runtime](bool condition) -> outcome::result<void> {
      return runtime.vm_assert(condition);
    };

    return execute(runtime, params, caller_assert, canExec);
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Exec>(),
  };

}  // namespace fc::vm::actor::builtin::v0::init
