/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::actor::builtin::states {
  using state::StateTree;
  using StateTreePtr = std::shared_ptr<StateTree>;
  using primitives::address::Address;

  class StateManager {
   public:
    virtual ~StateManager() = default;

    virtual AccountActorStatePtr createAccountActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<AccountActorStatePtr> getAccountActorState()
        const = 0;

    virtual CronActorStatePtr createCronActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<CronActorStatePtr> getCronActorState() const = 0;

    virtual InitActorStatePtr createInitActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<InitActorStatePtr> getInitActorState() const = 0;

    virtual MarketActorStatePtr createMarketActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<MarketActorStatePtr> getMarketActorState()
        const = 0;

    virtual MinerActorStatePtr createMinerActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<MinerActorStatePtr> getMinerActorState() const = 0;

    virtual MultisigActorStatePtr createMultisigActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<MultisigActorStatePtr> getMultisigActorState()
        const = 0;

    virtual PaymentChannelActorStatePtr createPaymentChannelActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<PaymentChannelActorStatePtr>
    getPaymentChannelActorState() const = 0;

    virtual PowerActorStatePtr createPowerActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<PowerActorStatePtr> getPowerActorState() const = 0;

    virtual RewardActorStatePtr createRewardActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<RewardActorStatePtr> getRewardActorState()
        const = 0;

    virtual SystemActorStatePtr createSystemActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<SystemActorStatePtr> getSystemActorState()
        const = 0;

    virtual VerifiedRegistryActorStatePtr createVerifiedRegistryActorState(
        ActorVersion version) const = 0;
    virtual outcome::result<VerifiedRegistryActorStatePtr>
    getVerifiedRegistryActorState() const = 0;

    virtual outcome::result<void> commitState(
        const std::shared_ptr<State> &state) = 0;
  };
}  // namespace fc::vm::actor::builtin::states
