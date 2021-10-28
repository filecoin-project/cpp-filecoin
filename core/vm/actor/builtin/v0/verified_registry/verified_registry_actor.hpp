/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {
  using primitives::StoragePower;
  using primitives::address::Address;
  using states::DataCap;
  using states::VerifiedRegistryActorStatePtr;

  struct Construct : ActorMethodBase<1> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  struct AddVerifier : ActorMethodBase<2> {
    struct Params {
      Address address;
      DataCap allowance;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> addVerifier(
        const Runtime &runtime,
        VerifiedRegistryActorStatePtr &state,
        const Address &verifier,
        const DataCap &allowance);
  };
  CBOR_TUPLE(AddVerifier::Params, address, allowance)

  struct RemoveVerifier : ActorMethodBase<3> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  struct AddVerifiedClient : ActorMethodBase<4> {
    struct Params {
      Address address;
      DataCap allowance;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> addClient(const Runtime &runtime,
                                           VerifiedRegistryActorStatePtr &state,
                                           const Address &client,
                                           const DataCap &allowance);
  };
  CBOR_TUPLE(AddVerifiedClient::Params, address, allowance)

  struct UseBytes : ActorMethodBase<5> {
    struct Params {
      Address address;
      StoragePower deal_size;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> useBytes(Runtime &runtime,
                                          VerifiedRegistryActorStatePtr &state,
                                          const Address &client,
                                          const StoragePower &deal_size);
  };
  CBOR_TUPLE(UseBytes::Params, address, deal_size)

  struct RestoreBytes : ActorMethodBase<6> {
    struct Params {
      Address address;
      StoragePower deal_size;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> restoreBytes(
        const Runtime &runtime,
        VerifiedRegistryActorStatePtr &state,
        const Address &client,
        const StoragePower &deal_size);
  };
  CBOR_TUPLE(RestoreBytes::Params, address, deal_size)

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::verified_registry
