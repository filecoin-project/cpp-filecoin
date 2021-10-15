/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v2/monies.hpp"

#include "vm/actor/actor_method.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using common::math::kPrecision128;
  using types::miner::kRewardVestingSpecV1;

  outcome::result<TokenAmount> Monies::expectedRewardForPower(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const ChainEpoch &projection_duration) const {
    const BigInt network_qa_power_smoothed = estimate(network_power_estimate);
    if (network_qa_power_smoothed == 0) {
      return estimate(reward_estimate);
    }
    const BigInt expected_reward_for_proving_period = extrapolatedCumSumOfRatio(
        projection_duration, 0, reward_estimate, network_power_estimate);
    const BigInt br128 = sector_power * expected_reward_for_proving_period;
    const BigInt br = br128 >> kPrecision128;
    return std::max(br, BigInt(0));
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForContinuedFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    return expectedRewardForPower(reward_estimate,
                                  network_power_estimate,
                                  sector_power,
                                  continued_fault_projection_period);
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForTerminationLowerBound(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    return expectedRewardForPower(
        reward_estimate,
        network_power_estimate,
        sector_power,
        termination_penalty_lower_bound_projection_period);
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
    const ChainEpoch lifetime_cap =
        ChainEpoch(termination_lifetime_cap) * kEpochsInDay;
    const ChainEpoch capped_sector_age = std::min(sector_age, lifetime_cap);

    TokenAmount expected_reward = day_reward * capped_sector_age;

    const ChainEpoch relevant_replaced_age =
        std::min(replaced_sector_age, lifetime_cap - capped_sector_age);
    expected_reward += replaced_day_reward * relevant_replaced_age;

    const TokenAmount penalized_reward =
        expected_reward * termination_reward_factor.numerator;

    OUTCOME_TRY(penalty,
                pledgePenaltyForTerminationLowerBound(
                    reward_estimate, network_power_estimate, sector_power));

    return std::max(
        penalty,
        BigInt(BigInt(twenty_day_reward_activation)
               + bigdiv(BigInt(penalized_reward),
                        (BigInt(kEpochsInDay))
                            * termination_reward_factor.denominator)));
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
    OUTCOME_TRY(ipBase,
                expectedRewardForPower(reward_estimate,
                                       network_power_estimate,
                                       power,
                                       initial_pledge_projection_period));

    const BigInt lock_target_num = initial_pledge_lock_target.numerator
                                   * network_circulation_supply_smoothed;
    const BigInt lock_target_denom = initial_pledge_lock_target.denominator;

    const StoragePower pledge_share_num = power;
    const StoragePower network_qa_power = estimate(network_power_estimate);
    const BigInt pledge_share_denom =
        std::max(std::max(network_qa_power, baseline_power), power);
    const BigInt additional_ip_num = lock_target_num * pledge_share_num;
    const BigInt additional_ip_denom = lock_target_denom * pledge_share_denom;
    const BigInt additional_ip = bigdiv(additional_ip_num, additional_ip_denom);

    const BigInt nominal_pledge = ipBase + additional_ip;
    const BigInt space_race_pledge_cap = initial_pledge_max_per_byte * power;
    return std::min(nominal_pledge, space_race_pledge_cap);
  }

  outcome::result<TokenAmount> Monies::repayDebtsOrAbort(
      Runtime &runtime, MinerActorStatePtr &miner_state) const {
    OUTCOME_TRY(curr_balance, runtime.getCurrentBalance());
    REQUIRE_NO_ERROR_A(to_burn,
                       miner_state->repayDebts(curr_balance),
                       VMExitCode::kErrIllegalState);
    return std::move(to_burn);
  }

  outcome::result<TokenAmount> Monies::consensusFaultPenalty(
      const TokenAmount &this_epoch_reward) const {
    return TokenAmount{this_epoch_reward * consensus_fault_factor
                       / expected_leader_per_epoch};
  }

  outcome::result<std::pair<TokenAmount, VestSpec>>
  Monies::lockedRewardFromReward(const TokenAmount &reward,
                                 const NetworkVersion &network_version) const {
    TokenAmount lock_amount = reward;
    if (network_version >= NetworkVersion::kVersion6) {
      lock_amount =
          reward
          * bigdiv(locked_reward_factor_num_v6, locked_reward_factor_denom_v6);
    }

    return std::make_pair(lock_amount, kRewardVestingSpecV1);
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForDeclaredFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const NetworkVersion &network_version) const {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::pledgePenaltyForUndeclaredFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power,
      const NetworkVersion &network_version) const {
    return TokenAmount{};
  }
}  // namespace fc::vm::actor::builtin::v2::miner
