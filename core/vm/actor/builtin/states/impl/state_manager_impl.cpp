/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/impl/state_manager_impl.hpp"

#include <utility>
#include "vm/actor/builtin/states/all_states.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::states {
  using toolchain::Toolchain;

  StateManagerImpl::StateManagerImpl(const IpldPtr &ipld,
                                     StateTreePtr state_tree,
                                     Address receiver)
      : ipld(ipld),
        state_tree(std::move(state_tree)),
        receiver(std::move(receiver)),
        provider(ipld) {}

  // Account Actor State
  //============================================================================

  AccountActorStatePtr StateManagerImpl::createAccountActorState(
      ActorVersion version) const {
    return createStatePtr<AccountActorState,
                          v0::account::AccountActorState,
                          v2::account::AccountActorState,
                          v3::account::AccountActorState>(version);
  }

  outcome::result<AccountActorStatePtr> StateManagerImpl::getAccountActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getAccountActorState(actor);
  }

  // Cron Actor State
  //============================================================================

  CronActorStatePtr StateManagerImpl::createCronActorState(
      ActorVersion version) const {
    return createStatePtr<CronActorState,
                          v0::cron::CronActorState,
                          v2::cron::CronActorState,
                          v3::cron::CronActorState>(version);
  }

  outcome::result<CronActorStatePtr> StateManagerImpl::getCronActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getCronActorState(actor);
  }

  // Init Actor State
  //============================================================================

  InitActorStatePtr StateManagerImpl::createInitActorState(
      ActorVersion version) const {
    return createStatePtr<InitActorState,
                          v0::init::InitActorState,
                          v2::init::InitActorState,
                          v3::init::InitActorState>(version);
  }

  outcome::result<InitActorStatePtr> StateManagerImpl::getInitActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getInitActorState(actor);
  }

  // Market Actor State
  //============================================================================

  MarketActorStatePtr StateManagerImpl::createMarketActorState(
      ActorVersion version) const {
    return createStatePtr<MarketActorState,
                          v0::market::MarketActorState,
                          v2::market::MarketActorState,
                          v2::market::MarketActorState>(
        version);  // TODO change v3
  }

  outcome::result<MarketActorStatePtr> StateManagerImpl::getMarketActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getMarketActorState(actor);
  }

  // Miner Actor State
  //============================================================================

  MinerActorStatePtr StateManagerImpl::createMinerActorState(
      ActorVersion version) const {
    return createStatePtr<MinerActorState,
                          v0::miner::MinerActorState,
                          v2::miner::MinerActorState,
                          v3::miner::MinerActorState>(version);
  }

  outcome::result<MinerActorStatePtr> StateManagerImpl::getMinerActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getMinerActorState(actor);
  }

  // Multisig Actor State
  //============================================================================

  MultisigActorStatePtr StateManagerImpl::createMultisigActorState(
      ActorVersion version) const {
    return createStatePtr<MultisigActorState,
                          v0::multisig::MultisigActorState,
                          v2::multisig::MultisigActorState,
                          v3::multisig::MultisigActorState>(version);
  }

  outcome::result<MultisigActorStatePtr>
  StateManagerImpl::getMultisigActorState() const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getMultisigActorState(actor);
  }

  // Payment Channel Actor State
  //============================================================================

  PaymentChannelActorStatePtr StateManagerImpl::createPaymentChannelActorState(
      ActorVersion version) const {
    return createStatePtr<PaymentChannelActorState,
                          v0::payment_channel::PaymentChannelActorState,
                          v2::payment_channel::PaymentChannelActorState,
                          v3::payment_channel::PaymentChannelActorState>(
        version);
  }

  outcome::result<PaymentChannelActorStatePtr>
  StateManagerImpl::getPaymentChannelActorState() const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getPaymentChannelActorState(actor);
  }

  // Power Actor State
  //============================================================================

  PowerActorStatePtr StateManagerImpl::createPowerActorState(
      ActorVersion version) const {
    return createStatePtr<PowerActorState,
                          v0::storage_power::PowerActorState,
                          v2::storage_power::PowerActorState,
                          v3::storage_power::PowerActorState>(version);
  }

  outcome::result<PowerActorStatePtr> StateManagerImpl::getPowerActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getPowerActorState(actor);
  }

  // Reward Actor State
  //============================================================================

  RewardActorStatePtr StateManagerImpl::createRewardActorState(
      ActorVersion version) const {
    return createStatePtr<RewardActorState,
                          v0::reward::RewardActorState,
                          v2::reward::RewardActorState,
                          v2::reward::RewardActorState>(
        version);  // TODO change to v3
  }

  outcome::result<RewardActorStatePtr> StateManagerImpl::getRewardActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getRewardActorState(actor);
  }

  // System Actor State
  //============================================================================

  SystemActorStatePtr StateManagerImpl::createSystemActorState(
      ActorVersion version) const {
    return createStatePtr<SystemActorState,
                          v0::system::SystemActorState,
                          v2::system::SystemActorState,
                          v3::system::SystemActorState>(version);
  }

  outcome::result<SystemActorStatePtr> StateManagerImpl::getSystemActorState()
      const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getSystemActorState(actor);
  }

  // Verified Registry Actor State
  //============================================================================

  VerifiedRegistryActorStatePtr
  StateManagerImpl::createVerifiedRegistryActorState(
      ActorVersion version) const {
    return createStatePtr<VerifiedRegistryActorState,
                          v0::verified_registry::VerifiedRegistryActorState,
                          v2::verified_registry::VerifiedRegistryActorState,
                          v3::verified_registry::VerifiedRegistryActorState>(
        version);
  }

  outcome::result<VerifiedRegistryActorStatePtr>
  StateManagerImpl::getVerifiedRegistryActorState() const {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    return provider.getVerifiedRegistryActorState(actor);
  }

  //============================================================================

  outcome::result<void> StateManagerImpl::commitState(
      const std::shared_ptr<State> &state) {
    switch (state->type) {
      case ActorType::kAccount:
        return commitVersionState<v0::account::AccountActorState,
                                  v2::account::AccountActorState,
                                  v3::account::AccountActorState>(state);

      case ActorType::kCron:
        return commitVersionState<v0::cron::CronActorState,
                                  v2::cron::CronActorState,
                                  v3::cron::CronActorState>(state);

      case ActorType::kInit:
        return commitVersionState<v0::init::InitActorState,
                                  v2::init::InitActorState,
                                  v3::init::InitActorState>(state);

      case ActorType::kMarket:
        return commitVersionState<v0::market::MarketActorState,
                                  v2::market::MarketActorState,
                                  v2::market::MarketActorState>(
            state);  // TODO v3

      case ActorType::kMiner:
        return commitVersionState<v0::miner::MinerActorState,
                                  v2::miner::MinerActorState,
                                  v3::miner::MinerActorState>(state);

      case ActorType::kMultisig:
        return commitVersionState<v0::multisig::MultisigActorState,
                                  v2::multisig::MultisigActorState,
                                  v3::multisig::MultisigActorState>(state);

      case ActorType::kPaymentChannel:
        return commitVersionState<
            v0::payment_channel::PaymentChannelActorState,
            v2::payment_channel::PaymentChannelActorState,
            v3::payment_channel::PaymentChannelActorState>(state);

      case ActorType::kPower:
        return commitVersionState<v0::storage_power::PowerActorState,
                                  v2::storage_power::PowerActorState,
                                  v3::storage_power::PowerActorState>(state);

      case ActorType::kReward:
        return commitVersionState<v0::reward::RewardActorState,
                                  v2::reward::RewardActorState,
                                  v2::reward::RewardActorState>(
            state);  // TODO v3

      case ActorType::kSystem:
        return commitVersionState<v0::system::SystemActorState,
                                  v2::system::SystemActorState,
                                  v3::system::SystemActorState>(state);

      case ActorType::kVerifiedRegistry:
        return commitVersionState<
            v0::verified_registry::VerifiedRegistryActorState,
            v2::verified_registry::VerifiedRegistryActorState,
            v3::verified_registry::VerifiedRegistryActorState>(state);
    }
  }

  outcome::result<void> StateManagerImpl::commit(const CID &new_state) {
    OUTCOME_TRY(actor, state_tree->get(receiver));
    actor.head = new_state;
    OUTCOME_TRY(state_tree->set(receiver, actor));
    return outcome::success();
  }
}  // namespace fc::vm::actor::builtin::states
