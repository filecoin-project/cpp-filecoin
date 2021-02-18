/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/state_manager.hpp"

#include <utility>
#include "vm/actor/builtin/states/all_states.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::states {
  using toolchain::Toolchain;

  StateManager::StateManager(const IpldPtr &ipld,
                             StateTreePtr state_tree,
                             Address receiver)
      : ipld(ipld),
        state_tree(std::move(state_tree)),
        receiver(std::move(receiver)),
        provider(ipld) {}

  AccountActorStatePtr StateManager::createAccountActorState(
      ActorVersion version) const {
    return createStatePtr<AccountActorState,
                          v0::account::AccountActorState,
                          v2::account::AccountActorState,
                          v3::account::AccountActorState>(version);
  }

  outcome::result<AccountActorStatePtr> StateManager::getAccountActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getAccountActorState(actor);
  }

  outcome::result<void> StateManager::commitState(
      const std::shared_ptr<State> &state) {
    switch (state->type) {
      case ActorType::kAccount:
        return commitVersionState<v0::account::AccountActorState,
                                  v2::account::AccountActorState,
                                  v3::account::AccountActorState>(state);
      default:
        return outcome::success();
    }
  }

  outcome::result<void> StateManager::commit(const CID &new_state) {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    actor.head = new_state;
    OUTCOME_TRY(state_tree->set(receiver, actor));
    return outcome::success();
  }
}  // namespace fc::vm::actor::builtin::states
