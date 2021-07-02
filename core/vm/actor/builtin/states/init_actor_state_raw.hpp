/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor {
  struct InitActorStateRaw {
    using Address = primitives::address::Address;
    using Hamt = storage::hamt::Hamt;

    CID _address_map;
    Hamt address_map{nullptr, 0, false};
    uint64_t next_id{};
    std::string network_name;

    void load(const IpldPtr &ipld, const CID &_code) {
      // TODO(turuslan): asActorCode from PR#415
      const auto code{common::span::bytestr(*asIdentity(_code))};
      const auto v3{code != code::init0 && code != code::init2};
      address_map = {ipld, _address_map, storage::hamt::kDefaultBitWidth, v3};
    }

    static outcome::result<Address> addActor(Hamt &hamt,
                                             uint64_t &next_id,
                                             const Address &address) {
      const auto id = next_id;
      OUTCOME_TRY(hamt.setCbor(adt::AddressKeyer::encode(address), id));
      ++next_id;
      return Address::makeFromId(id);
    }
    auto addActor(const Address &address) {
      return addActor(address_map, next_id, address);
    }

    static auto tryGet(Hamt &hamt, const Address &address) {
      return hamt.tryGetCbor<uint64_t>(adt::AddressKeyer::encode(address));
    }
    auto tryGet(const Address &address) {
      return tryGet(address_map, address);
    }
  };
  CBOR_TUPLE(InitActorStateRaw, _address_map, next_id, network_name)
}  // namespace fc::vm::actor

namespace fc {
  template <>
  struct Ipld::Flush<vm::actor::InitActorStateRaw> {
    static outcome::result<void> call(vm::actor::InitActorStateRaw &state) {
      OUTCOME_TRYA(state._address_map, state.address_map.flush());
      return outcome::success();
    }
  };
}  // namespace fc
