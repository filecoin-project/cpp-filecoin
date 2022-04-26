/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  struct AccountActor {
    enum class Method : MethodNumber {
      kConstruct = 1,
      kPubkeyAddress,
    }

    struct Construct : ActorMethodBase<Method::kConstruct> {
      using Params = Address;
    };

    struct PubkeyAddress : ActorMethodBase<Method::kPubkeyAddress> {
      using Result = Address;
    };
  };

}  // namespace fc::vm::actor::builtin
