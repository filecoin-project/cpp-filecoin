/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP
#define CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP

#include "common/outcome2.hpp"
#include "primitives/types.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::state {
  using actor::Actor;
  using primitives::ActorId;
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
  class StateTree : public std::enable_shared_from_this<StateTree> {
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
    virtual outcome::result<boost::optional<Actor>> tryGet(
        const Address &address) const = 0;

    outcome::result<Actor> get(const Address &address) const {
      OUTCOME_TRY(actor, tryGet(address));
      if (actor) {
        return *actor;
      }
      return OutcomeError::kDefault;
    }

    /// Lookup id address from address
    virtual outcome::result<boost::optional<Address>> tryLookupId(
        const Address &address) const = 0;

    outcome::result<Address> lookupId(const Address &address) const {
      OUTCOME_TRY(id, tryLookupId(address));
      if (id) {
        return *id;
      }
      return OutcomeError::kDefault;
    }

    /// Allocate id address and set actor state, does not write to storage
    virtual outcome::result<Address> registerNewAddress(
        const Address &address) = 0;
    /// Write changes to storage
    virtual outcome::result<CID> flush() = 0;

    /// Get store
    virtual std::shared_ptr<IpfsDatastore> getStore() const = 0;
    virtual outcome::result<void> remove(const Address &address) = 0;
    virtual void txBegin() = 0;
    virtual void txRevert() = 0;
    virtual void txEnd() = 0;

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
