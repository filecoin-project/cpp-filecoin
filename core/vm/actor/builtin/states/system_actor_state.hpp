/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::actor::builtin::states {

  struct SystemActorState : State {
    explicit SystemActorState(ActorVersion version)
        : State(ActorType::kSystem, version) {}
    // empty state
  };

  using SystemActorStatePtr = std::shared_ptr<SystemActorState>;
}  // namespace fc::vm::actor::builtin::states
