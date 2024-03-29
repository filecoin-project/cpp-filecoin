/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::address::Address;

  /// Init actor state
  struct InitActorState {
    adt::Map<uint64_t, adt::AddressKeyer> address_map;
    uint64_t next_id{};
    std::string network_name;

    /// Allocate new id address
    inline outcome::result<Address> addActor(const Address &address) {
      const auto id = next_id;
      OUTCOME_TRY(address_map.set(address, id));
      ++next_id;
      return Address::makeFromId(id);
    }
  };

  using InitActorStatePtr = types::Universal<InitActorState>;

}  // namespace fc::vm::actor::builtin::states
