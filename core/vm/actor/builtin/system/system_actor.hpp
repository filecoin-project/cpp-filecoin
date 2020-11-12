/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_SYSTEM_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_SYSTEM_ACTOR_HPP

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::system {

  /// System actor state
  struct State {
      //todo
      std::string s;
    // empty state
  };
  //todo
  CBOR_TUPLE(State, s)

  struct Construct : ActorMethodBase<1> {
    struct Params {
        //todo
        std::string s;
    };
    ACTOR_METHOD_DECL();
  };
  //todo
  CBOR_TUPLE(Construct::Params, s)

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::system

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_SYSTEM_ACTOR_HPP
