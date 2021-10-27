/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include <utility>

#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/dvm/dvm.hpp"

namespace fc::vm::state {
  using actor::builtin::states::InitActorStatePtr;
  using actor::builtin::types::miner::kChainFinality;

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store)
      : version_{StateTreeVersion::kVersion0}, store_{store}, by_id_{store} {
    // txBegin() is virtual and should not be used in the constructor
    tx_.emplace_back();
  }

  StateTreeImpl::StateTreeImpl(std::shared_ptr<IpfsDatastore> store,
                               const CID &root)
      : version_{StateTreeVersion::kVersion0}, store_{std::move(store)} {
    setRoot(root);
    // txBegin() is virtual and should not be used in the constructor
    tx_.emplace_back();
  }

  outcome::result<void> StateTreeImpl::set(const Address &address,
                                           const Actor &actor) {
    OUTCOME_TRY(address_id, lookupId(address));
    dvm::onActor(*this, address, actor);
    setActor(address_id.getId(), actor);
    return outcome::success();
  }

  outcome::result<boost::optional<Actor>> StateTreeImpl::tryGet(
      const Address &address) const {
    OUTCOME_TRY(id, tryLookupId(address));
    if (!id) {
      return boost::none;
    }
    for (auto it{tx_.rbegin()}; it != tx_.rend(); ++it) {
      if (it->removed.count(id->getId()) != 0) {
        return boost::none;
      }
      const auto actor{it->actors.find(id->getId())};
      if (actor != it->actors.end()) {
        return actor->second;
      }
    }
    OUTCOME_TRY(actor, by_id_.tryGet(*id));
    if (actor) {
      setActor(id->getId(), *actor);
    }
    return std::move(actor);
  }

  outcome::result<boost::optional<Address>> StateTreeImpl::tryLookupId(
      const Address &address) const {
    if (address.isId()) {
      return address;
    }
    for (auto it{tx_.rbegin()}; it != tx_.rend(); ++it) {
      const auto id{it->lookup.find(address)};
      if (id != it->lookup.end()) {
        return Address::makeFromId(id->second);
      }
    }
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(initActorState,
                getCbor<InitActorStatePtr>(store_, init_actor.head));
    OUTCOME_TRY(id, initActorState->address_map.tryGet(address));
    if (id) {
      tx_.back().lookup.emplace(address, *id);
      return Address::makeFromId(*id);
    }
    return boost::none;
  }

  outcome::result<Address> StateTreeImpl::registerNewAddress(
      const Address &address) {
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(state, getCbor<InitActorStatePtr>(store_, init_actor.head));
    OUTCOME_TRY(address_id, state->addActor(address));
    OUTCOME_TRYA(init_actor.head, setCbor(store_, state));
    OUTCOME_TRY(set(actor::kInitAddress, init_actor));
    return std::move(address_id);
  }

  outcome::result<CID> StateTreeImpl::flush() {
    assert(tx_.size() == 1);
    for (auto &[id, actor] : tx_.back().actors) {
      OUTCOME_TRY(by_id_.set(Address::makeFromId(id), actor));
    }
    for (const auto &id : tx_.back().removed) {
      OUTCOME_TRY(by_id_.remove(Address::makeFromId(id)));
    }
    OUTCOME_TRY(by_id_.hamt.flush());
    auto new_root = by_id_.hamt.cid();
    if (version_ == StateTreeVersion::kVersion0) {
      return new_root;
    }
    OUTCOME_TRY(info_cid, setCbor(store_, StateTreeInfo{}));
    return setCbor(store_, StateRoot{version_, new_root, info_cid});
  }

  std::shared_ptr<IpfsDatastore> StateTreeImpl::getStore() const {
    return store_;
  }

  outcome::result<void> StateTreeImpl::remove(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    tx_.back().removed.insert(address_id.getId());
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
    auto top{std::move(tx_.back())};
    tx_.pop_back();
    for (auto &[id, actor] : top.actors) {
      tx_.back().actors[id] = std::move(actor);
      tx_.back().removed.erase(id);
    }
    for (auto &[address, id] : top.lookup) {
      tx_.back().lookup[address] = id;
    }
    for (auto id : top.removed) {
      tx_.back().removed.insert(id);
    }
  }

  void StateTreeImpl::setActor(ActorId id, const Actor &actor) const {
    tx_.back().actors[id] = actor;
    tx_.back().removed.erase(id);
  }

  void StateTreeImpl::setRoot(const CID &root) {
    // Try load StateRoot as version >= 1
    if (auto _raw{store_->get(root)}) {
      auto &raw{_raw.value()};
      if (codec::cbor::CborDecodeStream{raw}.listLength() == 3) {
        if (auto _state_root{codec::cbor::decode<StateRoot>(_raw.value())}) {
          auto &state_root{_state_root.value()};
          version_ = state_root.version;
          by_id_.hamt = {
              store_,
              state_root.actor_tree_root,
              storage::hamt::kDefaultBitWidth,
          };
          return;
        }
      }
    }
    // if failed to load as version >= 1, must be version 0
    version_ = StateTreeVersion::kVersion0;
    by_id_ = {root, store_};
  }
}  // namespace fc::vm::state
