/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP

#include "primitives/address/address_codec.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::actor {
  using primitives::address::Address;
  using primitives::address::Protocol;
  using state::StateTree;

  struct AccountActorState {
    Address address;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const AccountActorState &state) {
    return s << (s.list() << state.address);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, AccountActorState &state) {
    s.list() >> state.address;
    return s;
  }

  struct AccountActor {
    static constexpr VMExitCode CREATE_WRONG_ADDRESS_TYPE{1};
    static constexpr VMExitCode RESOLVE_NOT_FOUND{1};
    static constexpr VMExitCode RESOLVE_NOT_ACCOUNT_ACTOR{1};

    /// Create account actor from BLS or Secp256k1 address
    static outcome::result<Address> create(
        const std::shared_ptr<StateTree> &state_tree, const Address &address);
    /// Get BLS address of account actor from ID address
    static outcome::result<Address> resolveToKeyAddress(
        const std::shared_ptr<StateTree> &state_tree, const Address &address);
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACCOUNT_ACTOR_HPP
