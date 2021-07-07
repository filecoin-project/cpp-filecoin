/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/init/init_actor_state.hpp"

#include "vm/actor/builtin/states/init_actor_state_raw.hpp"

namespace fc::vm::actor::builtin::v2::init {
  ACTOR_STATE_TO_CBOR_THIS(InitActorState)

  outcome::result<Address> InitActorState::addActor(const Address &address) {
    return InitActorStateRaw::addActor(address_map_0.hamt, next_id, address);
  }

  outcome::result<boost::optional<uint64_t>> InitActorState::tryGet(
      const Address &address) {
    return address_map_0.tryGet(address);
  }
}  // namespace fc::vm::actor::builtin::v2::init
