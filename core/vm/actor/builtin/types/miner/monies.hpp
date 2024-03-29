/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/version/version.hpp"

// Forward declaration
namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::types::miner {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using runtime::Runtime;
  using states::MinerActorStatePtr;
  using version::NetworkVersion;

  class Monies {
   public:
    constexpr static int precommit_deposit_factor = 20;
    constexpr static int initial_pledge_factor = 20;
    const ChainEpoch precommit_deposit_projection_period =
        ChainEpoch(precommit_deposit_factor) * kEpochsInDay;
    const ChainEpoch initial_pledge_projection_period =
        ChainEpoch(initial_pledge_factor) * kEpochsInDay;
    const ChainEpoch termination_lifetime_cap = ChainEpoch(70);

    virtual ~Monies() = default;

    virtual outcome::result<TokenAmount> expectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const ChainEpoch &projection_duration) const = 0;

    virtual outcome::result<TokenAmount> pledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) const = 0;

    virtual outcome::result<TokenAmount> pledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) const = 0;

    virtual outcome::result<TokenAmount> pledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        const ChainEpoch &sector_age,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version,
        const TokenAmount &day_reward,
        const TokenAmount &replaced_day_reward,
        const ChainEpoch &replaced_sector_age) const = 0;

    virtual outcome::result<TokenAmount> preCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) const = 0;

    virtual outcome::result<TokenAmount> initialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) const = 0;

    virtual outcome::result<TokenAmount> pledgePenaltyForContinuedFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) const = 0;

    virtual outcome::result<TokenAmount> pledgePenaltyForTerminationLowerBound(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) const = 0;

    virtual outcome::result<TokenAmount> repayDebtsOrAbort(
        Runtime &runtime, MinerActorStatePtr &miner_state) const = 0;

    virtual outcome::result<TokenAmount> consensusFaultPenalty(
        const TokenAmount &this_epoch_reward) const = 0;

    virtual outcome::result<std::pair<TokenAmount, VestSpec>>
    lockedRewardFromReward(const TokenAmount &reward,
                           const NetworkVersion &network_version) const = 0;

    virtual outcome::result<TokenAmount> pledgePenaltyForInvalidWindowPoSt(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) const = 0;
  };
}  // namespace fc::vm::actor::builtin::types::miner
