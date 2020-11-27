/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/init/init_actor.hpp"

#include "adt/address_key.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::actor::builtin::v0::init {
  bool canExec(const CID &caller_code_id, const CID &exec_code_id) {
    bool result = false;
    if (exec_code_id == kStorageMinerCodeCid) {
      result = caller_code_id == kStoragePowerCodeCid;
    } else if (exec_code_id == kPaymentChannelCodeCid
               || exec_code_id == kMultisigCodeCid) {
      result = true;
    }
    return result;
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    InitActorState state{};
    state.network_name = params.network_name;
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  outcome::result<Address> InitActorState::addActor(const Address &address) {
    auto id = next_id;
    OUTCOME_TRY(address_map.set(address, id));
    ++next_id;
    return Address::makeFromId(id);
  }

  ACTOR_METHOD_IMPL(Exec) {
    OUTCOME_TRY(caller_code_id,
                runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (!canExec(caller_code_id, params.code)) {
      return VMExitCode::kErrForbidden;
    }
    OUTCOME_TRY(actor_address, runtime.createNewActorAddress());

    OUTCOME_TRY(init_actor, runtime.getCurrentActorStateCbor<InitActorState>());
    OUTCOME_TRY(id_address, init_actor.addActor(actor_address));
    OUTCOME_TRY(runtime.commitState(init_actor));

    OUTCOME_TRY(runtime.createActor(id_address,
                                    Actor{params.code, kEmptyObjectCid, 0, 0}));
    OUTCOME_TRY(runtime.send(id_address,
                             kConstructorMethodNumber,
                             params.params,
                             runtime.getMessage().get().value));
    return Result{id_address, actor_address};
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Exec>(),
  };
}  // namespace fc::vm::actor::builtin::v0::init
