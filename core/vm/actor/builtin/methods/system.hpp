/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  struct SystemActor {
    enum class Method : MethodNumber {
      kConstruct = 1,
    }

    struct Construct : ActorMethodBase<Method::kConstruct> {};
  };

}  // namespace fc::vm::actor::builtin
