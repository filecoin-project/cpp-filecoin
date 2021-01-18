/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {

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

    static outcome::result<void> addVerifier(State &state,
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
                                           State &state,
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

    static outcome::result<void> useBytes(State &state,
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

    static outcome::result<void> restoreBytes(State &state,
                                              const Address &client,
                                              const StoragePower &deal_size);
  };
  CBOR_TUPLE(RestoreBytes::Params, address, deal_size)

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::verified_registry
