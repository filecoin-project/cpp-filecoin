/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/market/market_actor_utils.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/v3/miner/miner_actor.hpp"
#include "vm/actor/builtin/v3/reward/reward_actor.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor.hpp"
#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor.hpp"

namespace fc::vm::actor::builtin::v3::market {

  outcome::result<void> MarketUtils::checkCallers(
      const Address &provider) const {
    const auto caller = getRuntime().getImmediateCaller();
    OUTCOME_TRY(addresses, requestMinerControlAddress(provider));

    bool caller_ok = caller == addresses.worker;

    for (const auto &controller : addresses.control) {
      if (caller_ok) {
        break;
      }
      caller_ok = caller == controller;
    }

    if (!caller_ok) {
      ABORT(VMExitCode::kErrForbidden);
    }

    return outcome::success();
  }

  outcome::result<StoragePower> MarketUtils::getBaselinePowerFromRewardActor()
      const {
    OUTCOME_TRY(
        epoch_reward,
        getRuntime().sendM<reward::ThisEpochReward>(kRewardAddress, {}, 0));
    return epoch_reward.this_epoch_baseline_power;
  }

  outcome::result<std::tuple<StoragePower, StoragePower>>
  MarketUtils::getRawAndQaPowerFromPowerActor() const {
    OUTCOME_TRY(current_power,
                getRuntime().sendM<storage_power::CurrentTotalPower>(
                    kStoragePowerAddress, {}, 0));
    return std::make_tuple(current_power.raw_byte_power,
                           current_power.quality_adj_power);
  }

  outcome::result<void> MarketUtils::callVerifRegUseBytes(
      const Universal<DealProposal> &deal) const {
    OUTCOME_TRY(getRuntime().sendM<verified_registry::UseBytes>(
        kVerifiedRegistryAddress, {deal->client, uint64_t{deal->piece_size}}, 0));
    return outcome::success();
  }

  outcome::result<void> MarketUtils::callVerifRegRestoreBytes(
      const Universal<DealProposal> &deal) const {
    OUTCOME_TRY(getRuntime().sendM<verified_registry::RestoreBytes>(
        kVerifiedRegistryAddress, {deal->client, uint64_t{deal->piece_size}}, 0));
    return outcome::success();
  }

  outcome::result<Controls> MarketUtils::requestMinerControlAddress(
      const Address &miner) const {
    OUTCOME_TRY(addresses,
                getRuntime().sendM<miner::ControlAddresses>(miner, {}, 0));
    return Controls{.owner = addresses.owner,
                    .worker = addresses.worker,
                    .control = addresses.control};
  }

}  // namespace fc::vm::actor::builtin::v3::market
