/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/init/init_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::init {
  struct InitActorState : states::InitActorState {};
  CBOR_TUPLE(InitActorState, address_map, next_id, network_name)
}  // namespace fc::vm::actor::builtin::v0::init

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<fc::vm::actor::builtin::v0::init::InitActorState> {
    template <typename Visitor>
    static void call(fc::vm::actor::builtin::v0::init::InitActorState &state,
                     const Visitor &visit) {
      visit(state.address_map);
    }
  };
}  // namespace fc::cbor_blake
