/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/state/state_tree_error.hpp"

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
    kVersion1,
    // kVersion2 corresponds to actors >= v3.
    kVersion2,
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
    virtual outcome::result<boost::optional<Actor>> tryGet(
        const Address &address) const = 0;

    outcome::result<Actor> get(const Address &address) const {
      OUTCOME_TRY(actor, tryGet(address));
      if (actor) {
        return *actor;
      }
      return StateTreeError::kStateNotFound;
    }

    /// Lookup id address from address
    virtual outcome::result<boost::optional<Address>> tryLookupId(
        const Address &address) const = 0;

    /**
     * Resolves any address to the id address.
     * @param address to resolve
     * @return public key type address
     */
    outcome::result<Address> lookupId(const Address &address) const {
      OUTCOME_TRY(id, tryLookupId(address));
      if (id) {
        return *id;
      }
      return StateTreeError::kStateNotFound;
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
  };

  CBOR_TUPLE_0(StateTree::StateTreeInfo)
  CBOR_TUPLE(StateTree::StateRoot, version, actor_tree_root, info)

}  // namespace fc::vm::state
