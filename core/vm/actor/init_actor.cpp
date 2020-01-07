/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/init_actor.hpp"

#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"

namespace fc::vm::actor {
  outcome::result<Address> InitActorState::addActor(
      std::shared_ptr<IpfsDatastore> store, const Address &address) {
    storage::hamt::Hamt hamt(std::move(store), address_map);
    auto id = next_id;
    OUTCOME_TRY(hamt.setCbor(primitives::address::encodeToString(address), id));
    OUTCOME_TRY(cid, hamt.flush());
    address_map = cid;
    ++next_id;
    return Address::makeFromId(id);
  }
}  // namespace fc::vm::actor
