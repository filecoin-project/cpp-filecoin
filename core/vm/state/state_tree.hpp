/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP
#define CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::state {

  using actor::Actor;
  using primitives::address::Address;
  using storage::ipfs::IpfsDatastore;

  /// State tree
  class StateTree {
   public:
    virtual ~StateTree() = default;

    /// Set actor state, does not write to storage
    virtual outcome::result<void> set(const Address &address,
                                      const Actor &actor) = 0;

    /// Get actor state
    virtual outcome::result<Actor> get(const Address &address) = 0;

    /// Lookup id address from address
    virtual outcome::result<Address> lookupId(const Address &address) = 0;

    /// Allocate id address and set actor state, does not write to storage
    virtual outcome::result<Address> registerNewAddress(
        const Address &address) = 0;
    /// Write changes to storage
    virtual outcome::result<CID> flush() = 0;

    /// Revert changes to last flushed state
    virtual outcome::result<void> revert(const CID &root) = 0;

    /// Get store
    virtual std::shared_ptr<IpfsDatastore> getStore() = 0;

    /// Get decoded actor state
    template <typename T>
    outcome::result<T> state(const Address &address) {
      OUTCOME_TRY(actor, get(address));
      return getStore()->template getCbor<T>(actor.head);
    }
  };
}  // namespace fc::vm::state

#endif  // CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP
