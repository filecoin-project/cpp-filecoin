/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"

namespace fc::vm::actor::builtin::v0::init {
  using primitives::address::Address;

  /// Init actor state
  struct InitActorState {
    /// Allocate new id address
    outcome::result<Address> addActor(const Address &address);

    adt::Map<uint64_t, adt::AddressKeyer> address_map;
    uint64_t next_id{};
    std::string network_name;
  };
  CBOR_TUPLE(InitActorState, address_map, next_id, network_name)

}  // namespace fc::vm::actor::builtin::v0::init
