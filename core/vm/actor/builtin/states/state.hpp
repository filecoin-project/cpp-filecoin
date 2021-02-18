/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::states {
  struct State {
    State(ActorType type, ActorVersion version)
        : type(type), version(version) {}
    virtual ~State() = default;

    ActorType type;
    ActorVersion version;
  };
}  // namespace fc::vm::actor::builtin::states
