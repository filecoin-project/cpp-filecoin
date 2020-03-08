/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_IMPL_HPP

#include "filecoin/vm/state/state_tree.hpp"

#include "filecoin/storage/hamt/hamt.hpp"
#include "filecoin/vm/actor/actor.hpp"

namespace fc::vm::state {

  using actor::Actor;
  using primitives::address::Address;
  using storage::hamt::Hamt;
  using storage::ipfs::IpfsDatastore;

  /// State tree
  class StateTreeImpl : public StateTree {
   public:
    explicit StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store);
    StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store, const CID &root);
    /// Set actor state, does not write to storage
    outcome::result<void> set(const Address &address,
                              const Actor &actor) override;
    /// Get actor state
    outcome::result<Actor> get(const Address &address) override;
    /// Lookup id address from address
    outcome::result<Address> lookupId(const Address &address) override;
    /// Allocate id address and set actor state, does not write to storage
    outcome::result<Address> registerNewAddress(const Address &address,
                                                const Actor &actor) override;
    /// Write changes to storage
    outcome::result<CID> flush() override;
    /// Revert changes to last flushed state
    outcome::result<void> revert() override;
    /// Get store
    std::shared_ptr<IpfsDatastore> getStore() override;

   private:
    std::shared_ptr<IpfsDatastore> store_;
    Hamt hamt_, snapshot_;
  };
}  // namespace fc::vm::state

#endif  // CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_IMPL_HPP
