/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "common/error_text.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::StoragePower;
  using primitives::address::Address;
  using DataCap = primitives::StoragePower;

  struct VerifiedRegistryActorState : State {
    Address root_key;
    adt::Map<DataCap, adt::AddressKeyer> verifiers;
    adt::Map<DataCap, adt::AddressKeyer> verified_clients;

    /**
     * @param address must be an id address
     * @return data cap for the given address
     */
    outcome::result<boost::optional<DataCap>> getVerifiedClientDataCap(
        const Address &address) {
      if (!address.isId()) {
        return ERROR_TEXT("Can only look up ID addresses");
      }
      return verified_clients.tryGet(address);
    }
  };

  using VerifiedRegistryActorStatePtr =
      std::shared_ptr<VerifiedRegistryActorState>;
}  // namespace fc::vm::actor::builtin::states
