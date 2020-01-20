/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/account_actor.hpp"

namespace fc::vm::actor {
  outcome::result<Address> AccountActor::create(
      std::shared_ptr<state::StateTree> state_tree, const Address &address) {
    if (!address.isKeyType()) {
      return CREATE_WRONG_ADDRESS_TYPE;
    }
    Actor actor{kAccountCodeCid, ActorSubstateCID{kEmptyObjectCid}, 0, 0};
    if (address.getProtocol() == Protocol::BLS) {
      OUTCOME_TRY(state,
                  state_tree->store()->setCbor(AccountActorState{address}));
      actor.head = ActorSubstateCID{state};
    }
    return state_tree->registerNewAddress(address, actor);
  }

  outcome::result<Address> AccountActor::resolveToKeyAddress(
      std::shared_ptr<state::StateTree> state_tree, const Address &address) {
    if (address.isKeyType()) {
      return address;
    }
    auto maybe_actor = state_tree->get(address);
    if (!maybe_actor) {
      return RESOLVE_NOT_FOUND;
    }
    auto actor = maybe_actor.value();
    if (actor.code != kAccountCodeCid) {
      return RESOLVE_NOT_ACCOUNT_ACTOR;
    }
    OUTCOME_TRY(account_actor_state,
                state_tree->store()->getCbor<AccountActorState>(actor.head));
    return account_actor_state.address;
  }
}  // namespace fc::vm::actor
