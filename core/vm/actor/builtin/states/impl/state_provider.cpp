/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/state_provider.hpp"

#include <utility>
#include "vm/actor/builtin/states/all_states.hpp"
#include "vm/toolchain/common_address_matcher.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::states {
  using toolchain::CommonAddressMatcher;
  using toolchain::Toolchain;

  StateProvider::StateProvider(IpldPtr ipld) : ipld(std::move(ipld)) {}

  ActorVersion StateProvider::getVersion(const CodeId &code) const {
    return Toolchain::getActorVersionForCid(code);
  }

  outcome::result<AccountActorStatePtr> StateProvider::getAccountActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isAccountActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<AccountActorState,
                             v0::account::AccountActorState,
                             v2::account::AccountActorState,
                             v3::account::AccountActorState,
                             v3::account::AccountActorState,          // TODO v4
                             v3::account::AccountActorState>(actor);  // TODO v5
  }

  outcome::result<CronActorStatePtr> StateProvider::getCronActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isCronActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<CronActorState,
                             v0::cron::CronActorState,
                             v2::cron::CronActorState,
                             v3::cron::CronActorState>(actor);
  }

  outcome::result<InitActorStatePtr> StateProvider::getInitActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isInitActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<InitActorState,
                             v0::init::InitActorState,
                             v2::init::InitActorState,
                             v3::init::InitActorState,
                             v3::init::InitActorState,          // TODO v4
                             v3::init::InitActorState>(actor);  // TODO v5
  }

  outcome::result<MarketActorStatePtr> StateProvider::getMarketActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isStorageMarketActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<MarketActorState,
                             v0::market::MarketActorState,
                             v2::market::MarketActorState,
                             v2::market::MarketActorState,          // TODO v3
                             v2::market::MarketActorState,          // TODO v4
                             v2::market::MarketActorState>(actor);  // TODO v5
  }

  outcome::result<MinerActorStatePtr> StateProvider::getMinerActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isStorageMinerActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<MinerActorState,
                             v0::miner::MinerActorState,
                             v2::miner::MinerActorState,
                             v3::miner::MinerActorState,
                             v3::miner::MinerActorState,          // TODO v4
                             v3::miner::MinerActorState>(actor);  // TODO v5
  }

  outcome::result<MultisigActorStatePtr> StateProvider::getMultisigActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isMultisigActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<MultisigActorState,
                             v0::multisig::MultisigActorState,
                             v2::multisig::MultisigActorState,
                             v3::multisig::MultisigActorState>(actor);
  }

  outcome::result<PaymentChannelActorStatePtr>
  StateProvider::getPaymentChannelActorState(const Actor &actor) const {
    if (!CommonAddressMatcher::isPaymentChannelActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<PaymentChannelActorState,
                             v0::payment_channel::PaymentChannelActorState,
                             v2::payment_channel::PaymentChannelActorState,
                             v3::payment_channel::PaymentChannelActorState>(
        actor);
  }

  outcome::result<PowerActorStatePtr> StateProvider::getPowerActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isStoragePowerActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<PowerActorState,
                             v0::storage_power::PowerActorState,
                             v2::storage_power::PowerActorState,
                             v3::storage_power::PowerActorState,
                             v3::storage_power::PowerActorState,  // TODO v4
                             v3::storage_power::PowerActorState>(
        actor);  // TODO v5
  }

  outcome::result<SystemActorStatePtr> StateProvider::getSystemActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isSystemActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<SystemActorState,
                             v0::system::SystemActorState,
                             v2::system::SystemActorState,
                             v3::system::SystemActorState>(actor);
  }

  outcome::result<RewardActorStatePtr> StateProvider::getRewardActorState(
      const Actor &actor) const {
    if (!CommonAddressMatcher::isRewardActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<RewardActorState,
                             v0::reward::RewardActorState,
                             v2::reward::RewardActorState,
                             v2::reward::RewardActorState,          // TODO v3
                             v2::reward::RewardActorState,          // TODO v4
                             v2::reward::RewardActorState>(actor);  // TODO v5
  }

  outcome::result<VerifiedRegistryActorStatePtr>
  StateProvider::getVerifiedRegistryActorState(const Actor &actor) const {
    if (!CommonAddressMatcher::isVerifiedRegistryActor(actor.code)) {
      return VMExitCode::kSysErrIllegalActor;
    }

    return getCommonStatePtr<VerifiedRegistryActorState,
                             v0::verified_registry::VerifiedRegistryActorState,
                             v2::verified_registry::VerifiedRegistryActorState,
                             v3::verified_registry::VerifiedRegistryActorState>(
        actor);
  }

}  // namespace fc::vm::actor::builtin::states
