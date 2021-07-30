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
    adt::Map<uint64_t, adt::AddressKeyer> address_map;
    uint64_t next_id{};
    std::string network_name;

    /// Allocate new id address
    outcome::result<Address> addActor(const Address &address) {
      const auto id = next_id;
      OUTCOME_TRY(address_map.set(address, id));
      ++next_id;
      return Address::makeFromId(id);
    }

    outcome::result<boost::optional<uint64_t>> tryGet(const Address &address) {
      return address_map.tryGet(address);
    }
  };
  CBOR_TUPLE(InitActorState, address_map, next_id, network_name)

  using InitActorStatePtr = types::Universal<InitActorState>;

}  // namespace fc::vm::actor::builtin::states

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<fc::vm::actor::builtin::states::InitActorState> {
    template <typename Visitor>
    static void call(fc::vm::actor::builtin::states::InitActorState &state,
                     const Visitor &visit) {
      visit(state.address_map);
    }
  };
}  // namespace fc::cbor_blake
