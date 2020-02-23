/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/account/account_actor.hpp"

#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::account {

  outcome::result<Actor> AccountActor::create(
      const std::shared_ptr<StateTree> &state_tree, const Address &address) {
    if (!address.isKeyType()) {
      return VMExitCode::ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE;
    }
    Actor actor{kAccountCodeCid, ActorSubstateCID{kEmptyObjectCid}, 0, 0};
    if (address.getProtocol() == Protocol::BLS) {
      OUTCOME_TRY(state,
                  state_tree->getStore()->setCbor(AccountActorState{address}));
      actor.head = ActorSubstateCID{state};
    }
    OUTCOME_TRY(state_tree->registerNewAddress(address, actor));
    return actor;
  }

  outcome::result<Address> AccountActor::resolveToKeyAddress(
      const std::shared_ptr<StateTree> &state_tree, const Address &address) {
    if (address.isKeyType()) {
      return address;
    }
    auto maybe_actor = state_tree->get(address);
    if (!maybe_actor) {
      return VMExitCode::ACCOUNT_ACTOR_RESOLVE_NOT_FOUND;
    }
    auto actor = maybe_actor.value();
    if (actor.code != kAccountCodeCid) {
      return VMExitCode::ACCOUNT_ACTOR_RESOLVE_NOT_ACCOUNT_ACTOR;
    }
    OUTCOME_TRY(account_actor_state,
                state_tree->getStore()->getCbor<AccountActorState>(actor.head));
    return account_actor_state.address;
  }

  ACTOR_METHOD(pubkeyAddress) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<AccountActorState>());
    OUTCOME_TRY(result, codec::cbor::encode(state.address));
    return InvocationOutput{Buffer{result}};
  }

  const ActorExports exports = {
      {kPubkeyAddressMethodNumber, ActorMethod(pubkeyAddress)},
  };
}  // namespace fc::vm::actor::builtin::account
