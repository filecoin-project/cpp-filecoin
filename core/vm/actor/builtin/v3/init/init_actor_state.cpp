/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/init/init_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::init {
  outcome::result<Buffer> InitActorState::toCbor() const {
    return Ipld::encode(*this);
  }

  outcome::result<Address> InitActorState::addActor(const Address &address) {
    return _addActor(address, true);
  }

  outcome::result<boost::optional<uint64_t>> InitActorState::tryGet(
      const Address &address) {
    return address_map_3.tryGet(address);
  }
}  // namespace fc::vm::actor::builtin::v3::init
