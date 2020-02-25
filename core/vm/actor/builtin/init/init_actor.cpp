/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/init/init_actor.hpp"

#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/runtime/gas_cost.hpp"

namespace fc::vm::actor::builtin::init {

  outcome::result<Address> InitActorState::addActor(
      std::shared_ptr<IpfsDatastore> store, const Address &address) {
    storage::hamt::Hamt hamt(std::move(store), address_map);
    auto id = next_id;
    OUTCOME_TRY(hamt.setCbor(primitives::address::encodeToString(address), id));
    OUTCOME_TRY(hamt.flush());
    address_map = hamt.cid();
    ++next_id;
    return Address::makeFromId(id);
  }

  ACTOR_METHOD(exec) {
    OUTCOME_TRY(exec_params, decodeActorParams<ExecParams>(params));
    if (!isBuiltinActor(exec_params.code)) {
      return VMExitCode::INIT_ACTOR_NOT_BUILTIN_ACTOR;
    }
    if (isSingletonActor(exec_params.code)) {
      return VMExitCode::INIT_ACTOR_SINGLETON_ACTOR;
    }
    OUTCOME_TRY(runtime.chargeGas(runtime::kInitActorExecCost));
    auto &message = runtime.getMessage().get();
    auto actor_address{Address::makeActorExec(
        Buffer{primitives::address::encode(message.from)}.putUint64(
            message.nonce))};
    auto store = runtime.getIpfsDatastore();
    OUTCOME_TRY(init_actor, runtime.getCurrentActorStateCbor<InitActorState>());
    OUTCOME_TRY(id_address, init_actor.addActor(store, actor_address));
    OUTCOME_TRY(runtime.createActor(
        id_address,
        Actor{exec_params.code, ActorSubstateCID{kEmptyObjectCid}, 0, 0}));
    OUTCOME_TRY(runtime.send(id_address,
                             kConstructorMethodNumber,
                             exec_params.params,
                             message.value));
    OUTCOME_TRY(runtime.commitState(init_actor));
    return InvocationOutput{Buffer{primitives::address::encode(id_address)}};
  }

  const ActorExports exports{{kExecMethodNumber, ActorMethod(exec)}};
}  // namespace fc::vm::actor::builtin::init
