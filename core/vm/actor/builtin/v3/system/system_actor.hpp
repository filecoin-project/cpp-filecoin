/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V3_SYSTEM_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V3_SYSTEM_ACTOR_HPP

#include "vm/actor/builtin/v2/system/system_actor.hpp"

namespace fc::vm::actor::builtin::v3::system {

  /**
   * System actor v2 is identical to System actor v0
   */
  using State = v2::system::State;
  using Construct = v2::system::Construct;

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::system

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V3_SYSTEM_ACTOR_HPP
