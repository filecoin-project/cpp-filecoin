/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::StoragePower;
  using primitives::address::Address;
  using DataCap = primitives::StoragePower;

  struct VerifiedRegistryActorState : State {
    explicit VerifiedRegistryActorState(ActorVersion version)
        : State(ActorType::kVerifiedRegistry, version) {}

    Address root_key;
    adt::Map<DataCap, adt::AddressKeyer> verifiers;
    adt::Map<DataCap, adt::AddressKeyer> verified_clients;
  };

  using VerifiedRegistryActorStatePtr =
      std::shared_ptr<VerifiedRegistryActorState>;
}  // namespace fc::vm::actor::builtin::states
