/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/state_provider.hpp"

#include <utility>
#include "vm/actor/builtin/states/all_states.hpp"
#include "vm/toolchain/common_address_matcher.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::states {
  using toolchain::CommonAddressMatcher;
  using toolchain::Toolchain;

  StateProvider::StateProvider(IpldPtr ipld) : ipld(std::move(ipld)) {}

  ActorVersion StateProvider::getVersion(const CodeId &code) const {
    return Toolchain::getActorVersionForCid(code);
  }

  outcome::result<AccountActorStatePtr> StateProvider::getAccountActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isAccountActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<AccountActorState,
                             v0::account::AccountActorState,
                             v2::account::AccountActorState,
                             v3::account::AccountActorState>(actor);
  }
}  // namespace fc::vm::actor::builtin::states
