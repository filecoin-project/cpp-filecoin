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

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    if (!params.isKeyType()) {
      return VMExitCode::ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE;
    }
    OUTCOME_TRY(runtime.commitState(AccountActorState{params}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PubkeyAddress) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<AccountActorState>());
    return state.address;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<PubkeyAddress>(),
  };
}  // namespace fc::vm::actor::builtin::account
