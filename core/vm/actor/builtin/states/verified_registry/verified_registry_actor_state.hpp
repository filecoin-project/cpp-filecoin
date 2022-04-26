/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "common/error_text.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::DataCap;
  using primitives::StoragePower;
  using primitives::address::Address;

  struct VerifiedRegistryActorState {
    virtual ~VerifiedRegistryActorState() = default;

    Address root_key;
    adt::Map<DataCap, adt::AddressKeyer> verifiers;
    adt::Map<DataCap, adt::AddressKeyer> verified_clients;

    /**
     * @param address must be an id address
     * @return data cap of the client for the given address
     */
    inline outcome::result<boost::optional<DataCap>> getVerifiedClientDataCap(
        const Address &address) const {
      if (!address.isId()) {
        return ERROR_TEXT("Can only look up ID addresses");
      }
      return verified_clients.tryGet(address);
    }

    /**
     * @param address must be an id address
     * @return data cap of the verifier for the given address
     */
    inline outcome::result<boost::optional<DataCap>> getVerifierDataCap(
        const Address &address) const {
      if (!address.isId()) {
        return ERROR_TEXT("Can only look up ID addresses");
      }
      return verifiers.tryGet(address);
    }
  };

  using VerifiedRegistryActorStatePtr =
      types::Universal<VerifiedRegistryActorState>;

}  // namespace fc::vm::actor::builtin::states
