/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"

#include "vm/actor/builtin/states/account_actor_state.hpp"
#include "vm/actor/builtin/states/cron_actor_state.hpp"
#include "vm/actor/builtin/states/init_actor_state.hpp"
#include "vm/actor/builtin/states/market_actor_state.hpp"
#include "vm/actor/builtin/states/miner_actor_state.hpp"
#include "vm/actor/builtin/states/multisig_actor_state.hpp"
#include "vm/actor/builtin/states/payment_channel_actor_state.hpp"
#include "vm/actor/builtin/states/power_actor_state.hpp"
#include "vm/actor/builtin/states/reward_actor_state.hpp"
#include "vm/actor/builtin/states/system_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry_actor_state.hpp"

#include "vm/actor/builtin/v4/todo.hpp"

namespace fc::vm::actor::builtin::states {

  class StateProvider {
   public:
    explicit StateProvider(IpldPtr ipld);

    outcome::result<AccountActorStatePtr> getAccountActorState(
        const Actor &actor) const;

    outcome::result<CronActorStatePtr> getCronActorState(
        const Actor &actor) const;

    outcome::result<InitActorStatePtr> getInitActorState(
        const Actor &actor) const;

    outcome::result<MarketActorStatePtr> getMarketActorState(
        const Actor &actor) const;

    outcome::result<MinerActorStatePtr> getMinerActorState(
        const Actor &actor) const;

    outcome::result<MultisigActorStatePtr> getMultisigActorState(
        const Actor &actor) const;

    outcome::result<PaymentChannelActorStatePtr> getPaymentChannelActorState(
        const Actor &actor) const;

    outcome::result<PowerActorStatePtr> getPowerActorState(
        const Actor &actor) const;

    outcome::result<SystemActorStatePtr> getSystemActorState(
        const Actor &actor) const;

    outcome::result<RewardActorStatePtr> getRewardActorState(
        const Actor &actor) const;

    outcome::result<VerifiedRegistryActorStatePtr>
    getVerifiedRegistryActorState(const Actor &actor) const;

   private:
    ActorVersion getVersion(const CodeId &code) const;

    template <typename T>
    outcome::result<std::shared_ptr<T>> getStatePtr(const CID &head) const {
      OUTCOME_TRY(state, getCbor<T>(ipld, head));
      return std::make_shared<T>(state);
    }

    template <typename T,
              typename Tv0,
              typename Tv2,
              typename Tv3,
              typename Tv4 = void,
              typename Tv5 = void>
    outcome::result<std::shared_ptr<T>> getCommonStatePtr(
        const Actor &actor) const {
      const auto version = getVersion(actor.code);

      std::shared_ptr<T> state;
      switch (version) {
        case ActorVersion::kVersion0: {
          OUTCOME_TRYA(state, getStatePtr<Tv0>(actor.head));
          break;
        }
        case ActorVersion::kVersion2: {
          OUTCOME_TRYA(state, getStatePtr<Tv2>(actor.head));
          break;
        }
        case ActorVersion::kVersion3: {
          OUTCOME_TRYA(state, getStatePtr<Tv3>(actor.head));
          break;
        }
        case ActorVersion::kVersion4: {
          if constexpr (std::is_same_v<Tv4, void>) {
            TODO_ACTORS_V4();
          } else {
            OUTCOME_TRYA(state, getStatePtr<Tv4>(actor.head));
          }
          break;
        }
        case ActorVersion::kVersion5: {
          if constexpr (std::is_same_v<Tv5, void>) {
            TODO_ACTORS_V5();
          } else {
            OUTCOME_TRYA(state, getStatePtr<Tv5>(actor.head));
          }
          break;
        }
      }
      return state;
    }

    IpldPtr ipld;
  };
}  // namespace fc::vm::actor::builtin::states
