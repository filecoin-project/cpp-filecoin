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
#include "vm/actor/builtin/states/multisig_actor_state.hpp"
#include "vm/actor/builtin/states/payment_channel_actor_state.hpp"
#include "vm/actor/builtin/states/reward_actor_state.hpp"
#include "vm/actor/builtin/states/system_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry_actor_state.hpp"

namespace fc::vm::actor::builtin::states {

  class StateProvider {
   public:
    explicit StateProvider(IpldPtr ipld);
    ~StateProvider() = default;

    outcome::result<AccountActorStatePtr> getAccountActorState(
        const Actor &actor) const;

    outcome::result<CronActorStatePtr> getCronActorState(
        const Actor &actor) const;

    outcome::result<InitActorStatePtr> getInitActorState(
        const Actor &actor) const;

    outcome::result<MultisigActorStatePtr> getMultisigActorState(
        const Actor &actor) const;

    outcome::result<PaymentChannelActorStatePtr> getPaymentChannelActorState(
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
      OUTCOME_TRY(state, ipld->getCbor<T>(head));
      return std::make_shared<T>(state);
    }

    template <typename T, typename Tv0, typename Tv2, typename Tv3>
    outcome::result<std::shared_ptr<T>> getCommonStatePtr(
        const Actor &actor) const {
      const auto version = getVersion(actor.code);

      switch (version) {
        case ActorVersion::kVersion0: {
          OUTCOME_TRY(state, getStatePtr<Tv0>(actor.head));
          return std::static_pointer_cast<T>(state);
        }
        case ActorVersion::kVersion2: {
          OUTCOME_TRY(state, getStatePtr<Tv2>(actor.head));
          return std::static_pointer_cast<T>(state);
        }
        case ActorVersion::kVersion3: {
          OUTCOME_TRY(state, getStatePtr<Tv3>(actor.head));
          return std::static_pointer_cast<T>(state);
        }
      }
    }

    IpldPtr ipld;
  };
}  // namespace fc::vm::actor::builtin::states
