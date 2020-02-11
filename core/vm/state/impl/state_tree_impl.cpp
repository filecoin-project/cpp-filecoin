/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include "codec/cbor/cbor.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"

namespace fc::vm::state {
  using actor::ActorSubstateCID;
  using codec::cbor::decode;
  using primitives::address::encodeToString;

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store)
      : store_(store), hamt_(store), snapshot_(store) {}

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store,
                               const CID &root)
      : store_(store), hamt_(store, root), snapshot_(store, root) {}

  outcome::result<void> StateTreeImpl::set(const Address &address,
                                           const Actor &actor) {
    OUTCOME_TRY(address_id, lookupId(address));
    return hamt_.setCbor(encodeToString(address_id), actor);
  }

  outcome::result<Actor> StateTreeImpl::get(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    return hamt_.getCbor<Actor>(encodeToString(address_id));
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
    OUTCOME_TRY(id, address_map.getCbor<uint64_t>(encodeToString(address)));
    return Address::makeFromId(id);
  }

  outcome::result<Address> StateTreeImpl::registerNewAddress(
      const Address &address, const Actor &actor) {
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(
        init_actor_state,
        store_->getCbor<actor::builtin::init::InitActorState>(init_actor.head));
    OUTCOME_TRY(address_id, init_actor_state.addActor(store_, address));
    OUTCOME_TRY(init_actor_state_cid, store_->setCbor(init_actor_state));
    init_actor.head = ActorSubstateCID{init_actor_state_cid};
    OUTCOME_TRY(set(actor::kInitAddress, init_actor));
    OUTCOME_TRY(set(address_id, actor));
    return std::move(address_id);
  }

  outcome::result<CID> StateTreeImpl::flush() {
    OUTCOME_TRY(cid, hamt_.flush());
    snapshot_ = hamt_;
    return std::move(cid);
  }

  outcome::result<void> StateTreeImpl::revert() {
    hamt_ = snapshot_;
    return outcome::success();
  }

  std::shared_ptr<IpfsDatastore> StateTreeImpl::getStore() {
    return store_;
  }
}  // namespace fc::vm::state
