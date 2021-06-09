/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/state_manager.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::actor::builtin::states {
  using state::StateTree;
  using StateTreePtr = std::shared_ptr<StateTree>;
  using primitives::address::Address;

  class StateManagerImpl : public StateManager {
   public:
    StateManagerImpl(const IpldPtr &ipld,
                     StateTreePtr state_tree,
                     Address receiver);

    AccountActorStatePtr createAccountActorState(
        ActorVersion version) const override;
    outcome::result<AccountActorStatePtr> getAccountActorState() const override;

    CronActorStatePtr createCronActorState(ActorVersion version) const override;
    outcome::result<CronActorStatePtr> getCronActorState() const override;

    InitActorStatePtr createInitActorState(ActorVersion version) const override;
    outcome::result<InitActorStatePtr> getInitActorState() const override;

    MarketActorStatePtr createMarketActorState(
        ActorVersion version) const override;
    outcome::result<MarketActorStatePtr> getMarketActorState() const override;

    MinerActorStatePtr createMinerActorState(
        ActorVersion version) const override;
    outcome::result<MinerActorStatePtr> getMinerActorState() const override;

    MultisigActorStatePtr createMultisigActorState(
        ActorVersion version) const override;
    outcome::result<MultisigActorStatePtr> getMultisigActorState()
        const override;

    PaymentChannelActorStatePtr createPaymentChannelActorState(
        ActorVersion version) const override;
    outcome::result<PaymentChannelActorStatePtr> getPaymentChannelActorState()
        const override;

    PowerActorStatePtr createPowerActorState(
        ActorVersion version) const override;
    outcome::result<PowerActorStatePtr> getPowerActorState() const override;

    RewardActorStatePtr createRewardActorState(
        ActorVersion version) const override;
    outcome::result<RewardActorStatePtr> getRewardActorState() const override;

    SystemActorStatePtr createSystemActorState(
        ActorVersion version) const override;
    outcome::result<SystemActorStatePtr> getSystemActorState() const override;

    VerifiedRegistryActorStatePtr createVerifiedRegistryActorState(
        ActorVersion version) const override;
    outcome::result<VerifiedRegistryActorStatePtr>
    getVerifiedRegistryActorState() const override;

    outcome::result<void> commitState(
        const std::shared_ptr<State> &state) override;

   private:
    template <typename T,
              typename Tv0,
              typename Tv2,
              typename Tv3,
              typename Tv4 = void>
    std::shared_ptr<T> createStatePtr(ActorVersion version) const {
      switch (version) {
        case ActorVersion::kVersion0: {
          return createLoadedStatePtr<Tv0>();
        }
        case ActorVersion::kVersion2: {
          return createLoadedStatePtr<Tv2>();
        }
        case ActorVersion::kVersion3: {
          return createLoadedStatePtr<Tv3>();
        }
        case ActorVersion::kVersion4: {
          if constexpr (std::is_same_v<Tv4, void>) {
            TODO_ACTORS_V4();
          } else {
            return createLoadedStatePtr<Tv4>();
          }
        }
      }
    }

    template <typename T>
    std::shared_ptr<T> createLoadedStatePtr() const {
      auto state = std::make_shared<T>();
      ipld->load(*state);
      return state;
    }

    outcome::result<void> commit(const CID &new_state);

    IpldPtr ipld;
    StateTreePtr state_tree;
    Address receiver;
    StateProvider provider;

    LIBP2P_METRICS_INSTANCE_COUNT(
        fc::vm::actor::builtin::states::StateManagerImpl);
  };
}  // namespace fc::vm::actor::builtin::states
