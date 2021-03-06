/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/all_states.hpp"
#include "vm/actor/builtin/types/type_manager/universal_impl.hpp"

UNIVERSAL_IMPL(states::AccountActorState,
               v0::account::AccountActorState,
               v2::account::AccountActorState,
               v3::account::AccountActorState,
               v3::account::AccountActorState,
               v3::account::AccountActorState)

UNIVERSAL_IMPL(states::CronActorState,
               v0::cron::CronActorState,
               v2::cron::CronActorState,
               v3::cron::CronActorState,
               v3::cron::CronActorState,
               v3::cron::CronActorState)

UNIVERSAL_IMPL(states::InitActorState,
               v0::init::InitActorState,
               v2::init::InitActorState,
               v3::init::InitActorState,
               v3::init::InitActorState,
               v3::init::InitActorState)

UNIVERSAL_IMPL(states::MarketActorState,
               v0::market::MarketActorState,
               v2::market::MarketActorState,
               v2::market::MarketActorState,
               v2::market::MarketActorState,
               v2::market::MarketActorState)

UNIVERSAL_IMPL(states::MinerActorState,
               v0::miner::MinerActorState,
               v2::miner::MinerActorState,
               v3::miner::MinerActorState,
               v3::miner::MinerActorState,
               v3::miner::MinerActorState)

UNIVERSAL_IMPL(states::MultisigActorState,
               v0::multisig::MultisigActorState,
               v2::multisig::MultisigActorState,
               v3::multisig::MultisigActorState,
               v3::multisig::MultisigActorState,
               v3::multisig::MultisigActorState)

UNIVERSAL_IMPL(states::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v2::payment_channel::PaymentChannelActorState,
               v3::payment_channel::PaymentChannelActorState,
               v3::payment_channel::PaymentChannelActorState,
               v3::payment_channel::PaymentChannelActorState)

UNIVERSAL_IMPL(states::RewardActorState,
               v0::reward::RewardActorState,
               v2::reward::RewardActorState,
               v2::reward::RewardActorState,
               v2::reward::RewardActorState,
               v2::reward::RewardActorState)

UNIVERSAL_IMPL(states::PowerActorState,
               v0::storage_power::PowerActorState,
               v2::storage_power::PowerActorState,
               v3::storage_power::PowerActorState,
               v3::storage_power::PowerActorState,
               v3::storage_power::PowerActorState)

UNIVERSAL_IMPL(states::SystemActorState,
               v0::system::SystemActorState,
               v2::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState)

UNIVERSAL_IMPL(states::VerifiedRegistryActorState,
               v0::verified_registry::VerifiedRegistryActorState,
               v2::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState)
