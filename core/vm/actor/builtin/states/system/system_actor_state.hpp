/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/type_manager/universal.hpp"

namespace fc::vm::actor::builtin::states {

  struct SystemActorState {
    virtual ~SystemActorState() = default;
    // empty state
  };

  using SystemActorStatePtr = types::Universal<SystemActorState>;

}  // namespace fc::vm::actor::builtin::states
