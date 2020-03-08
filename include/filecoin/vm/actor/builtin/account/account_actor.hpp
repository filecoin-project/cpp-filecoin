/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/primitives/address/address_codec.hpp"
#include "filecoin/vm/actor/actor_method.hpp"
#include "filecoin/vm/state/state_tree.hpp"

namespace fc::vm::actor::builtin::account {

  using primitives::address::Address;
  using primitives::address::Protocol;
  using state::StateTree;

  struct AccountActorState {
    Address address;
  };
  CBOR_TUPLE(AccountActorState, address)

  /// Account actors represent actors without code
  struct AccountActor {
    /// Create account actor from BLS or Secp256k1 address
    static outcome::result<Actor> create(
        const std::shared_ptr<StateTree> &state_tree, const Address &address);
    /**
     * Get BLS address of account actor from ID address
     * @param state_tree state tree
     * @param address id address to be resolved to key address
     * @returns key address associated with id address
     */
    static outcome::result<Address> resolveToKeyAddress(
        const std::shared_ptr<StateTree> &state_tree, const Address &address);
  };

  struct PubkeyAddress : ActorMethodBase<2> {
    using Result = Address;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::account

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
