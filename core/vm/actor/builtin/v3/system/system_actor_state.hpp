/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/system_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::system {

  /// System actor state
  struct SystemActorState : states::SystemActorState {
    SystemActorState() : states::SystemActorState(ActorVersion::kVersion3) {}
  };
  CBOR_TUPLE_0(SystemActorState)

}  // namespace fc::vm::actor::builtin::v3::system
