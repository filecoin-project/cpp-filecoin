/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::address::Address;

  struct AccountActorState : State {
    explicit AccountActorState(ActorVersion version)
        : State(ActorType::kAccount, version) {}

    Address address;
  };

  using AccountActorStatePtr = std::shared_ptr<AccountActorState>;
}  // namespace fc::vm::actor::builtin::states
