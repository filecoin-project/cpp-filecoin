/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/state_manager.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
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
    template <typename T, typename Tv0, typename Tv2, typename Tv3>
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
      }
    }

    template <typename T>
    std::shared_ptr<T> createLoadedStatePtr() const {
      auto state = std::make_shared<T>();
      ipld->load(*state);
      return state;
    }

    template <typename T>
    outcome::result<void> commitCborState(const T &state) {
      OUTCOME_TRY(state_cid, ipld->setCbor(state));
      OUTCOME_TRY(commit(state_cid));
      return outcome::success();
    }

    template <typename Tv0, typename Tv2, typename Tv3>
    outcome::result<void> commitVersionState(
        const std::shared_ptr<State> &state) {
      switch (state->version) {
        case ActorVersion::kVersion0:
          return commitCborState(static_cast<const Tv0 &>(*state));
        case ActorVersion::kVersion2:
          return commitCborState(static_cast<const Tv2 &>(*state));
        case ActorVersion::kVersion3:
          return commitCborState(static_cast<const Tv3 &>(*state));
      }
    }

    outcome::result<void> commit(const CID &new_state);

    IpldPtr ipld;
    StateTreePtr state_tree;
    Address receiver;
    StateProvider provider;
  };
}  // namespace fc::vm::actor::builtin::states
