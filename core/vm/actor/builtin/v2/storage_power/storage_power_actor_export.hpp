/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_STORAGE_POWER_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_STORAGE_POWER_ACTOR_HPP

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {

  struct OnEpochTickEnd : ActorMethodBase<10> {
    ACTOR_METHOD_DECL();
  };

}  // namespace fc::vm::actor::builtin::v2::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_STORAGE_POWER_ACTOR_HPP
