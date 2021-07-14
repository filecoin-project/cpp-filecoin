/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::address::Address;

  /// Init actor state
  struct InitActorState {
    virtual ~InitActorState() = default;

    adt::Map<uint64_t, adt::AddressKeyer> address_map_0;
    adt::MapV3<uint64_t, adt::AddressKeyer> address_map_3;
    uint64_t next_id{};
    std::string network_name;

    /// Allocate new id address
    virtual outcome::result<Address> addActor(const Address &address) = 0;

    virtual outcome::result<boost::optional<uint64_t>> tryGet(
        const Address &address) = 0;
  };

  using InitActorStatePtr = types::Universal<InitActorState>;

}  // namespace fc::vm::actor::builtin::states
