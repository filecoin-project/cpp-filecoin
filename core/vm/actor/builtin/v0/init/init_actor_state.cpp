/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/init/init_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::init {
  outcome::result<Address> InitActorState::addActor(const Address &address) {
    const auto id = next_id;
    OUTCOME_TRY(address_map.set(address, id));
    ++next_id;
    return Address::makeFromId(id);
  }
}  // namespace fc::vm::actor::builtin::v0::init
