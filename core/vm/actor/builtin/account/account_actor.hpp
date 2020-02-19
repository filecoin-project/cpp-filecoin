/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::actor::builtin::account {

  using primitives::address::Address;
  using primitives::address::Protocol;
  using state::StateTree;

  struct AccountActorState {
    Address address;
  };

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

  CBOR_ENCODE(AccountActorState, state) {
    return s << (s.list() << state.address);
  }

  CBOR_DECODE(AccountActorState, state) {
    s.list() >> state.address;
    return s;
  }

}  // namespace fc::vm::actor::builtin::account

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
