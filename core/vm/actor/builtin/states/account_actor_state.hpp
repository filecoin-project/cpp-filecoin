/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

#include "primitives/address/address.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::address::Address;

  struct AccountActorState : State {
    Address address;
  };

  using AccountActorStatePtr = std::shared_ptr<AccountActorState>;
}  // namespace fc::vm::actor::builtin::states
