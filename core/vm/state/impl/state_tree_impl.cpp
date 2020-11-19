/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/dvm/dvm.hpp"

namespace fc::vm::state {
  using actor::builtin::v0::init::InitActorState;

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store)
      : store_{store}, by_id{store} {}

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store,
                               const CID &root)
      : store_{store} {
    setRoot(root);
  }

  outcome::result<void> StateTreeImpl::set(const Address &address,
                                           const Actor &actor) {
    OUTCOME_TRY(address_id, lookupId(address));
    dvm::onActor(*this, address, actor);
    return by_id.set(address_id, actor);
  }

  outcome::result<Actor> StateTreeImpl::get(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    return by_id.get(address_id);
  }

  outcome::result<Address> StateTreeImpl::lookupId(const Address &address) {
    if (address.isId()) {
      return address;
    }
    OUTCOME_TRY(init_actor_state, state<InitActorState>(actor::kInitAddress));
    OUTCOME_TRY(id, init_actor_state.address_map.get(address));
    return Address::makeFromId(id);
  }

  outcome::result<Address> StateTreeImpl::registerNewAddress(
      const Address &address) {
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(init_actor_state,
                store_->getCbor<InitActorState>(init_actor.head));
    OUTCOME_TRY(address_id, init_actor_state.addActor(address));
    OUTCOME_TRYA(init_actor.head, store_->setCbor(init_actor_state));
    OUTCOME_TRY(set(actor::kInitAddress, init_actor));
    return std::move(address_id);
  }

  outcome::result<CID> StateTreeImpl::flush() {
    OUTCOME_TRY(Ipld::flush(by_id));
    auto new_root = by_id.hamt.cid();
    OUTCOME_TRY(info_cid, store_->setCbor(StateTreeInfo{}));
    return store_->setCbor(StateRoot{.version = StateTreeVersion::kVersion1,
                                     .actor_tree_root = new_root,
                                     .info = info_cid});
  }

  outcome::result<void> StateTreeImpl::revert(const CID &root) {
    setRoot(root);
    return outcome::success();
  }

  std::shared_ptr<IpfsDatastore> StateTreeImpl::getStore() {
    return store_;
  }

  outcome::result<void> StateTreeImpl::remove(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    return by_id.remove(address_id);
  }

  void StateTreeImpl::setRoot(const CID &root) {
    // Try load StateRoot as version >= 1
    auto maybe_state_root = store_->getCbor<StateRoot>(root);
    if (maybe_state_root.has_value()) {
      by_id = {maybe_state_root.value().actor_tree_root, store_};
    } else {
      // if failed to load as version >= 1, must be version 0
      by_id = {root, store_};
    }
  }

}  // namespace fc::vm::state
