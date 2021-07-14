/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/system_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::v0::system {

  /// System actor state
  struct SystemActorState : states::SystemActorState {};
  CBOR_TUPLE_0(SystemActorState)

}  // namespace fc::vm::actor::builtin::v0::system
