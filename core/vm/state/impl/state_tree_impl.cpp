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
      : version_{StateTreeVersion::kVersion0}, store_{store}, by_id{store} {
    txBegin();
  }

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store,
                               const CID &root)
      : version_{StateTreeVersion::kVersion0}, store_{store} {
    setRoot(root).assume_value();
    txBegin();
  }

  outcome::result<void> StateTreeImpl::set(const Address &address,
                                           const Actor &actor) {
    OUTCOME_TRY(address_id, lookupId(address));
    dvm::onActor(*this, address, actor);
    _set(address_id.getId(), actor);
    return outcome::success();
  }

  outcome::result<Actor> StateTreeImpl::get(const Address &address) const {
    OUTCOME_TRY(address_id, lookupId(address));
    for (auto it{tx_.rbegin()}; it != tx_.rend(); ++it) {
      if (it->removed.count(address_id.getId())) {
        return storage::hamt::HamtError::kNotFound;
      }
      const auto actor{it->actors.find(address_id.getId())};
      if (actor != it->actors.end()) {
        return actor->second;
      }
    }
    OUTCOME_TRY(actor, by_id.get(address_id));
    _set(address_id.getId(), actor);
    return std::move(actor);
  }

  outcome::result<Address> StateTreeImpl::lookupId(const Address &address) const {
    if (address.isId()) {
      return address;
    }
    for (auto it{tx_.rbegin()}; it != tx_.rend(); ++it) {
      const auto id{it->lookup.find(address)};
      if (id != it->lookup.end()) {
        return Address::makeFromId(id->second);
      }
    }
    OUTCOME_TRY(init_actor_state, state<InitActorState>(actor::kInitAddress));
    OUTCOME_TRY(id, init_actor_state.address_map.get(address));
    tx().lookup.emplace(address, id);
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
    assert(tx_.size() == 1);
    for (auto &[id, actor] : tx().actors) {
      OUTCOME_TRY(by_id.set(Address::makeFromId(id), actor));
    }
    for (auto &id : tx().removed) {
      OUTCOME_TRY(by_id.remove(Address::makeFromId(id)));
    }
    OUTCOME_TRY(Ipld::flush(by_id));
    auto new_root = by_id.hamt.cid();
    if (version_ == StateTreeVersion::kVersion0) {
      return new_root;
    }
    OUTCOME_TRY(info_cid, store_->setCbor(StateTreeInfo{}));
    return store_->setCbor(StateRoot{
        .version = version_, .actor_tree_root = new_root, .info = info_cid});
  }

  std::shared_ptr<IpfsDatastore> StateTreeImpl::getStore() const {
    return store_;
  }

  outcome::result<void> StateTreeImpl::remove(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    tx().removed.insert(address_id.getId());
    return outcome::success();
  }

  void StateTreeImpl::txBegin() {
    tx_.emplace_back();
  }

  void StateTreeImpl::txRevert() {
    tx_.back() = {};
  }

  void StateTreeImpl::txEnd() {
    assert(tx_.size() > 1);
    auto top{std::move(tx())};
    tx_.pop_back();
    for (auto &[id, actor] : top.actors) {
      tx().actors[id] = std::move(actor);
      tx().removed.erase(id);
    }
    for (auto &[address, id] : top.lookup) {
      tx().lookup[address] = id;
    }
    for (auto id : top.removed) {
      tx().removed.insert(id);
    }
  }

  StateTreeImpl::Tx &StateTreeImpl::tx() const {
    return tx_.back();
  }

  void StateTreeImpl::_set(ActorId id, const Actor &actor) const {
    tx().actors[id] = actor;
    tx().removed.erase(id);
  }

  outcome::result<void> StateTreeImpl::setRoot(const CID &root) {
    OUTCOME_TRY(raw, store_->get(root));
    // Try load StateRoot as version >= 1
    if (codec::cbor::CborDecodeStream{raw}.listLength() == 3) {
      OUTCOME_TRY(state_root, codec::cbor::decode<StateRoot>(raw));
      version_ = state_root.version;
      by_id = {state_root.actor_tree_root, store_};
    } else {
      // if failed to load as version >= 1, must be version 0
      version_ = StateTreeVersion::kVersion0;
      by_id = {root, store_};
    }
    return outcome::success();
  }
}  // namespace fc::vm::state
