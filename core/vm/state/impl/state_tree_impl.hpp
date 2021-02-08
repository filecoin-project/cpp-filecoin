/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_IMPL_HPP

#include "vm/state/state_tree.hpp"

#include "adt/address_key.hpp"
#include "adt/map.hpp"

namespace fc::vm::state {
  /// State tree
  class StateTreeImpl : public StateTree {
   public:
    struct Tx {
      std::map<ActorId, Actor> actors;
      std::map<Address, ActorId> lookup;
      std::set<ActorId> removed;
    };

    explicit StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store);
    StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store, const CID &root);
    /// Set actor state, does not write to storage
    outcome::result<void> set(const Address &address,
                              const Actor &actor) override;
    /// Get actor state
    outcome::result<Actor> get(const Address &address) const override;
    /// Lookup id address from address
    outcome::result<Address> lookupId(const Address &address) const override;
    /// Allocate id address and set actor state, does not write to storage
    outcome::result<Address> registerNewAddress(
        const Address &address) override;
    /// Write changes to storage
    outcome::result<CID> flush() override;
    /// Get store
    std::shared_ptr<IpfsDatastore> getStore() const override;
    outcome::result<void> remove(const Address &address) override;
    void txBegin() override;
    void txRevert() override;
    void txEnd() override;

   private:
    Tx &tx() const;
    void _set(ActorId id, const Actor &actor) const;
    /**
     * Sets root of StateTree
     * @param root - cid of hamt for StateTree v0 or cid of struct StateRoot for
     * StateTree v1
     */
    outcome::result<void> setRoot(const CID &root);

    StateTreeVersion version_;
    std::shared_ptr<IpfsDatastore> store_;
    adt::Map<actor::Actor, adt::AddressKeyer> by_id;
    mutable std::vector<Tx> tx_;
  };
}  // namespace fc::vm::state

#endif  // CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_IMPL_HPP
