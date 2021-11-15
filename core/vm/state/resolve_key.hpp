/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/account/account_actor_state.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::state {
  /**
   * Given id address of account actor returns associated key address.
   * Given key address returns it unchanged.
   * Actor hash addresses have no key associated.
   */
  inline outcome::result<Address> resolveKey(StateTree &state_tree,
                                             const IpldPtr &charging_ipld,
                                             const Address &address) {
    if (address.isKeyType()) {
      return address;
    }
    if (!address.isId()) {
      return ERROR_TEXT("resolveKey hash address has no key");
    }
    if (const auto _actor{state_tree.get(address)}) {
      const auto &actor{_actor.value()};
      OUTCOME_TRY(state,
                  getCbor<actor::builtin::states::AccountActorStatePtr>(
                      charging_ipld, actor.head));
      if (!state->address.isKeyType()) {
        return ERROR_TEXT("resolveKey AccountActorState was not key");
      }
      return state->address;
    }
    return vm::VMExitCode::kSysErrIllegalArgument;
  }

  inline outcome::result<Address> resolveKey(StateTree &state_tree,
                                             const Address &address) {
    return resolveKey(state_tree, state_tree.getStore(), address);
  }
}  // namespace fc::vm::state

namespace fc {
  using vm::state::resolveKey;
}  // namespace fc
