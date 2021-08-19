/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "monies.hpp"

#include <vm/actor/actor_method.hpp>
#include <vm/actor/builtin/v2/miner/miner_actor.hpp>
#include <vm/actor/builtin/v2/multisig/multisig_actor.hpp>

namespace fc::vm::actor::builtin::v2::miner {
  outcome::result<TokenAmount> Monies::ExpectedRewardForPower(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const ChainEpoch &projection_duration) {
    BigInt network_qa_power_smoothed = estimate(network_qa_power_estimate);
    if (!network_qa_power_smoothed) {
      return estimate(reward_estimate);
    }
    fc::primitives::BigInt expected_reward_for_proving_period =
        extrapolatedCumSumOfRatio(
            projection_duration, 0, reward_estimate, network_qa_power_estimate);
    fc::primitives::BigInt br128 =
        qa_sector_power * expected_reward_for_proving_period;
    fc::primitives::BigInt br = br128 >> fc::common::math::kPrecision128;
    return std::max(br, fc::primitives::BigInt(0));
  }

  fc::outcome::result<fc::primitives::TokenAmount>
  vm::actor::builtin::v2::miner::Monies::PledgePenaltyForContinuedFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return ExpectedRewardForPower(reward_estimate,
                                  network_qa_power_estimate,
                                  qa_sector_power,
                                  continued_fault_projection_period);
  }

  fc::outcome::result<TokenAmount>
  vm::actor::builtin::v2::miner::Monies::PledgePenaltyForTerminationLowerBound(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return ExpectedRewardForPower(
        reward_estimate,
        network_qa_power_estimate,
        qa_sector_power,
        termination_penalty_lower_bound_projection_period);
  }

  fc::outcome::result<TokenAmount>
  vm::actor::builtin::v2::miner::Monies::PledgePenaltyForTermination(
      const TokenAmount &day_reward,
      const ChainEpoch &sector_age,
      const TokenAmount &twenty_day_reward_activation,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const FilterEstimate &reward_estimate,
      const TokenAmount &replaced_day_reward,
      const ChainEpoch &replaced_sector_age) {
    ChainEpoch lifetime_cap =
        ChainEpoch(termination_lifetime_cap) * fc::kEpochsInDay;
    ChainEpoch capped_sector_age = std::min(sector_age, lifetime_cap);

    TokenAmount expected_reward = day_reward * capped_sector_age;

    ChainEpoch relevant_replaced_age =
        std::min(replaced_sector_age, lifetime_cap - capped_sector_age);
    expected_reward =
        expected_reward + replaced_day_reward * relevant_replaced_age;

    TokenAmount penalized_reward =
        expected_reward * termination_reward_factor.numerator;

    return std::max(
        PledgePenaltyForTerminationLowerBound(
            reward_estimate, network_qa_power_estimate, qa_sector_power)
            .value(),
        fc::primitives::BigInt(
            fc::primitives::BigInt(twenty_day_reward_activation)
            + fc::primitives::BigInt(penalized_reward)
                  / (fc::primitives::BigInt(fc::kEpochsInDay)
                     * termination_reward_factor.denominator)));
  }

  fc::outcome::result<TokenAmount>
  vm::actor::builtin::v2::miner::Monies::PreCommitDepositForPower(
      const FilterEstimate &reward_estimate,
      FilterEstimate network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return ExpectedRewardForPower(reward_estimate,
                                  network_qa_power_estimate,
                                  qa_sector_power,
                                  precommit_deposit_projection_period);
  }

  fc::outcome::result<TokenAmount>
  vm::actor::builtin::v2::miner::Monies::InitialPledgeForPower(
      const StoragePower &qa_power,
      const StoragePower &baseline_power,
      const TokenAmount &network_total_pledge,
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const TokenAmount &network_circulation_supply_smoothed) {
    TokenAmount ipBase =
        ExpectedRewardForPower(reward_estimate,
                               network_qa_power_estimate,
                               qa_power,
                               initial_pledge_projection_period)
            .value();

    fc::primitives::BigInt lock_target_num =
        initial_pledge_lock_target.numerator
        * network_circulation_supply_smoothed;
    fc::primitives::BigInt lock_target_denom =
        initial_pledge_lock_target.denominator;

    auto pledge_share_num = qa_power;
    StoragePower network_qa_power = estimate(network_qa_power_estimate);
    fc::primitives::BigInt pledge_share_denom =
        std::max(std::max(network_qa_power, baseline_power), qa_power);
    fc::primitives::BigInt additional_ip_num =
        lock_target_num * pledge_share_num;
    fc::primitives::BigInt additional_ip_denom =
        lock_target_denom * pledge_share_denom;
    fc::primitives::BigInt additional_ip =
        additional_ip_num / additional_ip_denom;

    fc::primitives::BigInt nominal_pledge = ipBase + additional_ip;
    fc::primitives::BigInt space_race_pledge_cap =
        initial_pledge_max_per_byte * qa_power;
    return std::min(nominal_pledge, space_race_pledge_cap);
  }

  outcome::result<TokenAmount> Monies::RepayDebtsOrAbort(
      const fc::vm::runtime::Runtime &runtime,
      fc::vm::actor::builtin::states::MinerActorState *state) {
    auto curr_balance = runtime.getCurrentBalance();
    // TODO(m.tagirov): MinerSate.RepayDebts(curr_balance);
    return TokenAmount{1};
  }

  outcome::result<TokenAmount> Monies::ConsensusFaultPenalty(
      const TokenAmount &this_epoch_reward) {
    return this_epoch_reward * consensus_fault_factor
           / expected_leader_per_epoch;
  }

  outcome::result<std::pair<TokenAmount, VestSpec *>>
  Monies::LockedRewardFromReward(const TokenAmount &reward,
                                 const NetworkVersion &nv) {
    TokenAmount lock_amount = reward;
    VestSpec *spec = const_cast<VestSpec *>(&kRewardVestingSpecV1);
    if (nv >= version::NetworkVersion::kVersion6) {
      lock_amount =
          reward * locked_reward_factor_num_v6 / locked_reward_factor_denom_v6;
    }

    return {lock_amount, spec};
  }
  // override to default return value

  outcome::result<TokenAmount> Monies::ExpectedRewardForPower(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const ChainEpoch &projection_duration) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForDeclaredFault(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const NetworkVersion &network_version) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForUndeclaredFault(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const NetworkVersion &network_version) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForTermination(
      const TokenAmount &day_reward_at_activation,
      const TokenAmount &twenty_day_reward_activation,
      const ChainEpoch &sector_age,
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const NetworkVersion &network_version) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PreCommitDepositForPower(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::InitialPledgeForPower(
      const StoragePower &qa_power,
      const StoragePower &baseline_power,
      const TokenAmount &network_total_pledge,
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const TokenAmount &network_circulation_supply_smoothed) {
    return TokenAmount{};
  }
}  // namespace fc::vm::actor::builtin::v2::miner
