/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/init_actor.hpp"

#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/runtime/gas_cost.hpp"

namespace fc::vm::actor {
  outcome::result<Address> InitActorState::addActor(
      std::shared_ptr<IpfsDatastore> store, const Address &address) {
    storage::hamt::Hamt hamt(std::move(store), address_map);
    auto id = next_id;
    OUTCOME_TRY(hamt.setCbor(primitives::address::encodeToString(address), id));
    OUTCOME_TRY(cid, hamt.flush());
    address_map = cid;
    ++next_id;
    return Address::makeFromId(id);
  }

  outcome::result<InvocationOutput> InitActor::exec(
      const Actor &actor, Runtime &runtime, const MethodParams &params) {
    OUTCOME_TRY(exec_params, decodeActorParams<ExecParams>(params));
    if (!isBuiltinActor(exec_params.code)) {
      return InitActor::NOT_BUILTIN_ACTOR;
    }
    if (isSingletonActor(exec_params.code)) {
      return InitActor::SINGLETON_ACTOR;
    }
    OUTCOME_TRY(runtime.chargeGas(runtime::kInitActorExecCost));
    auto &message = runtime.getMessage().get();
    auto actor_address{Address::makeActorExecAddress(
        Buffer{primitives::address::encode(message.from)}.putUint64(
            message.nonce))};
    auto store = runtime.getIpfsDatastore();
    OUTCOME_TRY(init_actor, store->getCbor<InitActorState>(runtime.getHead()));
    OUTCOME_TRY(id_address, init_actor.addActor(store, actor_address));
    OUTCOME_TRY(runtime.createActor(
        id_address,
        Actor{exec_params.code, ActorSubstateCID{kEmptyObjectCid}, 0, 0}));
    OUTCOME_TRY(runtime.send(id_address,
                             kConstructorMethodNumber,
                             exec_params.params,
                             message.value));
    OUTCOME_TRY(new_head, store->setCbor(init_actor));
    OUTCOME_TRY(runtime.commit(ActorSubstateCID{new_head}));
    return InvocationOutput{Buffer{primitives::address::encode(id_address)}};
  }

  ActorExports InitActor::exports{
      {InitActor::kExecMethodNumber, ActorMethod(InitActor::exec)}};
}  // namespace fc::vm::actor
