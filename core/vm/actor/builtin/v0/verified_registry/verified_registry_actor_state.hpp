/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {
  using primitives::StoragePower;
  using primitives::address::Address;
  using DataCap = primitives::StoragePower;

  inline const StoragePower kMinVerifiedDealSize{1 << 20};

  struct State {
    Address root_key;
    adt::Map<DataCap, adt::AddressKeyer> verifiers;
    adt::Map<DataCap, adt::AddressKeyer> verified_clients;

    State() = default;
    explicit State(const Address &root) : root_key(root) {}
  };
  CBOR_TUPLE(State, root_key, verifiers, verified_clients)

}  // namespace fc::vm::actor::builtin::v0::verified_registry

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::verified_registry::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::verified_registry::State &state,
                     const Visitor &visit) {
      visit(state.verifiers);
      visit(state.verified_clients);
    }
  };
}  // namespace fc
