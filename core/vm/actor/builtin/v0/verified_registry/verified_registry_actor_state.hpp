/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/builtin/states/verified_registry_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {
  struct VerifiedRegistryActorState : states::VerifiedRegistryActorState {
    outcome::result<Buffer> toCbor() const override;
  };
  CBOR_TUPLE(VerifiedRegistryActorState, root_key, verifiers, verified_clients)
}  // namespace fc::vm::actor::builtin::v0::verified_registry

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<
      vm::actor::builtin::v0::verified_registry::VerifiedRegistryActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v0::verified_registry::VerifiedRegistryActorState
            &state,
        const Visitor &visit) {
      visit(state.verifiers);
      visit(state.verified_clients);
    }
  };
}  // namespace fc::cbor_blake
