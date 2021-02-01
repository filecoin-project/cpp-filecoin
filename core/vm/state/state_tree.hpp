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

  /**
   * StateTreeVersion is the version of the state tree itself, independent of
   * the network version or the actors version.
   */
  enum class StateTreeVersion : uint64_t {
    // kVersion0 corresponds to actors < v2.
    kVersion0,
    // kVersion1 corresponds to actors >= v2.
    kVersion1
  };

  /// State tree
  class StateTree {
   public:
    /** Used in StateRoot v1 */
    struct StateTreeInfo {
      // empty
    };

    /**
     * State tree stored in ipld as:
     *  - version 0 is sipmple cid
     *  - version >= 1 contains version number and additional info (StateRoot)
     */
    struct StateRoot {
      StateTreeVersion version;
      CID actor_tree_root;
      CID info;
    };

    virtual ~StateTree() = default;

    /// Set actor state, does not write to storage
    virtual outcome::result<void> set(const Address &address,
                                      const Actor &actor) = 0;

    /// Get actor state
    virtual outcome::result<Actor> get(const Address &address) const = 0;

    /// Lookup id address from address
    virtual outcome::result<Address> lookupId(const Address &address) const = 0;

    /// Allocate id address and set actor state, does not write to storage
    virtual outcome::result<Address> registerNewAddress(
        const Address &address) = 0;
    /// Write changes to storage
    virtual outcome::result<CID> flush() = 0;

    /// Revert changes to last flushed state
    virtual outcome::result<void> revert(const CID &root) = 0;

    /// Get store
    virtual std::shared_ptr<IpfsDatastore> getStore() const = 0;

    /// Get decoded actor state
    template <typename T>
    outcome::result<T> state(const Address &address) const {
      OUTCOME_TRY(actor, get(address));
      return getStore()->template getCbor<T>(actor.head);
    }
  };

  CBOR_TUPLE_0(StateTree::StateTreeInfo)
  CBOR_TUPLE(StateTree::StateRoot, version, actor_tree_root, info)

}  // namespace fc::vm::state

#endif  // CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP
