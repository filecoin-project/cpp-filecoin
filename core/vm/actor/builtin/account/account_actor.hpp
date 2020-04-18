/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::actor::builtin::account {

  using primitives::address::Address;
  using primitives::address::Protocol;
  using state::StateTree;

  struct AccountActorState {
    Address address;
  };
  CBOR_TUPLE(AccountActorState, address)

  struct Construct : ActorMethodBase<1> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  struct PubkeyAddress : ActorMethodBase<2> {
    using Result = Address;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::account

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
