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
    OUTCOME_TRY(state,
                state_tree->getStore()->setCbor(AccountActorState{address}));
    Actor actor{kAccountCodeCid, ActorSubstateCID{state}, 0, 0};
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

  ACTOR_METHOD_IMPL(PubkeyAddress) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<AccountActorState>());
    return state.address;
  }

  const ActorExports exports{
      exportMethod<PubkeyAddress>(),
  };
}  // namespace fc::vm::actor::builtin::account
