/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/account/account_actor_state.hpp"
#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/state/resolve_key.hpp"

namespace fc::api {
  using primitives::tipset::TipsetCPtr;
  using vm::state::StateTreeImpl;
  using InterpreterResult = vm::interpreter::Result;
  using primitives::address::Address;
  using vm::actor::kInitAddress;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::kVerifiedRegistryAddress;
  using vm::actor::builtin::states::AccountActorStatePtr;
  using vm::actor::builtin::states::InitActorStatePtr;
  using vm::actor::builtin::states::MarketActorStatePtr;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::states::PowerActorStatePtr;
  using vm::actor::builtin::states::RewardActorStatePtr;
  using vm::actor::builtin::states::VerifiedRegistryActorStatePtr;

  struct TipsetContext {
    TipsetCPtr tipset;
    StateTreeImpl state_tree;
    boost::optional<InterpreterResult> interpreted;

    auto marketState() const -> outcome::result<MarketActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kStorageMarketAddress));
      return getCbor<MarketActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto minerState(const Address &address) const
        -> outcome::result<MinerActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(address));
      return getCbor<MinerActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto powerState() const -> outcome::result<PowerActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kStoragePowerAddress));
      return getCbor<PowerActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto rewardState() const -> outcome::result<RewardActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(vm::actor::kRewardAddress));
      return getCbor<RewardActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto initState() const -> outcome::result<InitActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kInitAddress));
      return getCbor<InitActorStatePtr>(state_tree.getStore(), actor.head);
    }

    auto verifiedRegistryState() const
        -> outcome::result<VerifiedRegistryActorStatePtr> {
      OUTCOME_TRY(actor, state_tree.get(kVerifiedRegistryAddress));
      return getCbor<VerifiedRegistryActorStatePtr>(state_tree.getStore(),
                                                    actor.head);
    }

    outcome::result<Address> accountKey(const Address &id) const {
      return resolveKey(state_tree, id);
    }

    // TODO (a.chernyshov) make explicit
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator IpldPtr() const {
      return state_tree.getStore();
    }
  };

}  // namespace fc::api
