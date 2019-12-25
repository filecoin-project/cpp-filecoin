/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP
#define CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP

#include "storage/hamt/hamt.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::state {
  using actor::Actor;
  using libp2p::multi::ContentIdentifier;
  using primitives::address::Address;
  using storage::hamt::Hamt;
  using storage::ipfs::IpfsDatastore;

  class StateTree {
   public:
    StateTree(std::shared_ptr<IpfsDatastore> store);
    StateTree(std::shared_ptr<IpfsDatastore> store,
              const ContentIdentifier &root);
    outcome::result<void> set(const Address &address, const Actor &actor);
    outcome::result<Actor> get(const Address &address);
    outcome::result<Address> lookupId(const Address &address);
    outcome::result<Address> registerNewAddress(const Address &address,
                                                const Actor &actor);
    outcome::result<ContentIdentifier> flush();
    outcome::result<void> revert();

   private:
    std::shared_ptr<IpfsDatastore> store_;
    Hamt hamt_, snapshot_;
  };
}  // namespace fc::vm::state

#endif  // CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_HPP
