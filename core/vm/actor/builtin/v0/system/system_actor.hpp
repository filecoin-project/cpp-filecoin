/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V0_SYSTEM_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V0_SYSTEM_ACTOR_HPP

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::v0::system {

  /// System actor state
  struct State {
    // empty state
  };
  CBOR_TUPLE_0(State)

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v0::system

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V0_SYSTEM_ACTOR_HPP
