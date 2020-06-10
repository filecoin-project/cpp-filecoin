/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_VERIFIED_REGISTRY_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_VERIFIED_REGISTRY_ACTOR_HPP

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::verified_registry {
  using primitives::StoragePower;

  inline const StoragePower kMinVerifiedDealSize{1 << 20};

  struct State {
    Address root_key;
    adt::Map<StoragePower, adt::AddressKeyer> verifiers, verified_clients;
  };
  CBOR_TUPLE(State, root_key, verifiers, verified_clients)

  struct Constructor : ActorMethodBase<1> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  struct AddVerifier : ActorMethodBase<2> {
    struct Params {
      Address address;
      StoragePower allowance;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AddVerifier::Params, address, allowance)

  struct RemoveVerifier : ActorMethodBase<3> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  struct AddVerifiedClient : ActorMethodBase<4> {
    struct Params {
      Address address;
      StoragePower allowance;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AddVerifiedClient::Params, address, allowance)

  struct UseBytes : ActorMethodBase<5> {
    struct Params {
      Address address;
      StoragePower deal_size;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(UseBytes::Params, address, deal_size)

  struct RestoreBytes : ActorMethodBase<6> {
    struct Params {
      Address address;
      StoragePower deal_size;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(RestoreBytes::Params, address, deal_size)

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::verified_registry

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_VERIFIED_REGISTRY_ACTOR_HPP
