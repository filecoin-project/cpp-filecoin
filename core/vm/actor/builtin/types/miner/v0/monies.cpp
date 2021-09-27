/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v0/monies.hpp"

#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using common::math::kPrecision128;

  outcome::result<TokenAmount> Monies::expectedRewardForPower(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const ChainEpoch &projection_duration) const {
    const BigInt network_power_smoothed = estimate(network_power_estimate);
    if (network_power_smoothed == 0) {
      return estimate(reward_estimate);
    }
    const BigInt expected_reward_for_proving_period = extrapolatedCumSumOfRatio(
        projection_duration, 0, reward_estimate, network_power_estimate);
    const BigInt br = sector_power * expected_reward_for_proving_period;
    return br >> kPrecision128;
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForDeclaredFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const NetworkVersion &network_version) const {
    ChainEpoch projection_period = declared_fault_projection_period_v0;
    if (network_version >= NetworkVersion::kVersion3) {
      projection_period = declared_fault_projection_period_v3;
    }
    return expectedRewardForPower(reward_estimate,
                                  network_power_estimate,
                                  sector_power,
                                  projection_period);
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForUndeclaredFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const NetworkVersion &network_version) const {
    ChainEpoch projection_period = undeclared_fault_projection_period_v0;
    if (network_version >= NetworkVersion::kVersion1) {
      projection_period = undeclared_fault_projection_period_v1;
    }
    return expectedRewardForPower(reward_estimate,
                                  network_power_estimate,
                                  sector_power,
                                  projection_period);
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForTermination(
      const TokenAmount &day_reward_at_activation,
      const TokenAmount &twenty_day_reward_activation,
      const ChainEpoch &sector_age,
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const NetworkVersion &network_version,
      const TokenAmount &day_reward,
      const TokenAmount &replaced_day_reward,
      const ChainEpoch &replaced_sector_age) const {
    BigInt capped_sector_age = BigInt(std::min(
        BigInt(sector_age), BigInt(termination_lifetime_cap * kEpochsInDay)));
    if (network_version >= NetworkVersion::kVersion1) {
      capped_sector_age =
          BigInt(std::min(BigInt(sector_age / 2),
                          BigInt(termination_lifetime_cap * kEpochsInDay)));
    }
    OUTCOME_TRY(penalty,
                pledgePenaltyForUndeclaredFault(reward_estimate,
                                                network_power_estimate,
                                                sector_power,
                                                network_version));
    return std::max(
        penalty,
        BigInt(twenty_day_reward_activation
               + bigdiv(BigInt(day_reward_at_activation * capped_sector_age),
                        BigInt(kEpochsInDay))));
  }

  outcome::result<TokenAmount> Monies::preCommitDepositForPower(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    return expectedRewardForPower(reward_estimate,
                                  network_power_estimate,
                                  sector_power,
                                  precommit_deposit_projection_period);
  }

  outcome::result<TokenAmount> Monies::initialPledgeForPower(
      const StoragePower &power,
      const StoragePower &baseline_power,
      const TokenAmount &network_total_pledge,
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const TokenAmount &network_circulation_supply_smoothed) const {
    const StoragePower network_qa_power = estimate(network_power_estimate);
    OUTCOME_TRY(ipBase,
                expectedRewardForPower(reward_estimate,
                                       network_power_estimate,
                                       power,
                                       initial_pledge_projection_period));

    const BigInt lock_target_num =
        lock_target_factor_num * network_circulation_supply_smoothed;
    const BigInt lock_target_denom = lock_target_factor_denom;

    const StoragePower pledge_share_num = power;
    const BigInt pledge_share_denom =
        std::max(std::max(network_qa_power, baseline_power), power);
    const BigInt additional_ip_num = lock_target_num * pledge_share_num;
    const BigInt additional_ip_denom = lock_target_denom * pledge_share_denom;
    const BigInt additional_ip = bigdiv(additional_ip_num, additional_ip_denom);

    const BigInt nominal_pledge = ipBase + additional_ip;
    const BigInt space_race_pledge_cap =
        space_race_initial_pledge_max_per_byte * power;
    return std::min(nominal_pledge, space_race_pledge_cap);
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForContinuedFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForTerminationLowerBound(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::repayDebtsOrAbort(
      Runtime &runtime, MinerActorStatePtr miner_state) const {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::consensusFaultPenalty(
      const TokenAmount &this_epoch_reward) const {
    return TokenAmount{};
  }

  outcome::result<std::pair<TokenAmount, VestSpec>>
  Monies::lockedRewardFromReward(const TokenAmount &reward,
                                 const NetworkVersion &network_version) const {
    return std::make_pair(TokenAmount{}, VestSpec{});
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForInvalidWindowPoSt(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    return TokenAmount{};
  }

}  // namespace fc::vm::actor::builtin::v0::miner
