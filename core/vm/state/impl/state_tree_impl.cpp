/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include "adt/address_key.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"

namespace fc::vm::state {
  using actor::ActorSubstateCID;
  using adt::AddressKeyer;
  using codec::cbor::decode;

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store)
      : store_{store}, hamt_{store} {}

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store,
                               const CID &root)
      : store_{store}, hamt_{store, root} {}

  outcome::result<void> StateTreeImpl::set(const Address &address,
                                           const Actor &actor) {
    OUTCOME_TRY(address_id, lookupId(address));
    return hamt_.setCbor(AddressKeyer::encode(address_id), actor);
  }

  outcome::result<Actor> StateTreeImpl::get(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    return hamt_.getCbor<Actor>(AddressKeyer::encode(address_id));
  }

  outcome::result<Address> StateTreeImpl::lookupId(const Address &address) {
    if (address.getProtocol() == primitives::address::Protocol::ID) {
      return address;
    }
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(
        init_actor_state,
        store_->getCbor<actor::builtin::init::InitActorState>(init_actor.head));
    Hamt address_map(store_, init_actor_state.address_map);
    OUTCOME_TRY(id,
                address_map.getCbor<uint64_t>(AddressKeyer::encode(address)));
    return Address::makeFromId(id);
  }

  outcome::result<Address> StateTreeImpl::registerNewAddress(
      const Address &address) {
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(
        init_actor_state,
        store_->getCbor<actor::builtin::init::InitActorState>(init_actor.head));
    OUTCOME_TRY(address_id, init_actor_state.addActor(store_, address));
    OUTCOME_TRY(init_actor_state_cid, store_->setCbor(init_actor_state));
    init_actor.head = ActorSubstateCID{init_actor_state_cid};
    OUTCOME_TRY(set(actor::kInitAddress, init_actor));
    return std::move(address_id);
  }

  outcome::result<CID> StateTreeImpl::flush() {
    return hamt_.flush();
  }

  outcome::result<void> StateTreeImpl::revert(const CID &root) {
    hamt_ = {store_, root};
    return outcome::success();
  }

  std::shared_ptr<IpfsDatastore> StateTreeImpl::getStore() {
    return store_;
  }
}  // namespace fc::vm::state
