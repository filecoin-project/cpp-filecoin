/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/state/state_tree.hpp"

#include "adt/address_key.hpp"
#include "adt/map.hpp"

namespace fc::vm::state {
  /// State tree stores actor state by their address
  class StateTreeImpl : public StateTree {
   public:
    /// State snapshot layer stores changes that are not committed yet.
    struct Tx {
      std::map<ActorId, Actor> actors;
      std::map<Address, ActorId> lookup;
      std::set<ActorId> removed;
    };

    explicit StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store);
    StateTreeImpl(std::shared_ptr<IpfsDatastore> store, const CID &root);
    /// Set actor state, does not write to storage
    outcome::result<void> set(const Address &address,
                              const Actor &actor) override;
    /// Get actor state
    outcome::result<boost::optional<Actor>> tryGet(
        const Address &address) const override;
    /// Lookup id address from address
    outcome::result<boost::optional<Address>> tryLookupId(
        const Address &address) const override;
    /// Allocate id address and set actor state, does not write to storage
    outcome::result<Address> registerNewAddress(
        const Address &address) override;
    /// Write changes to storage
    outcome::result<CID> flush() override;
    /// Get store
    std::shared_ptr<IpfsDatastore> getStore() const override;
    outcome::result<void> remove(const Address &address) override;

    /// Creates new snapshot layer.
    void txBegin() override;

    /// Reverts snapshot layer to the previous one.
    void txRevert() override;

    /// Removes snapshot layer and merges changes to the previous layer.
    void txEnd() override;

   private:
    void setActor(ActorId id, const Actor &actor) const;
    /**
     * Sets root of StateTree
     * @param root - cid of hamt for StateTree v0 or cid of struct StateRoot for
     * StateTree v1
     */
    void setRoot(const CID &root);

    StateTreeVersion version_;
    std::shared_ptr<IpfsDatastore> store_;
    adt::Map<actor::Actor, adt::AddressKeyer> by_id_;
    mutable std::vector<Tx> tx_;
  };
}  // namespace fc::vm::state
