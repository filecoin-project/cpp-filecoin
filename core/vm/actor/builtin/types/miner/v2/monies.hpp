/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/v0/monies.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using common::math::kPrecision128;
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using runtime::Runtime;
  using states::MinerActorState;
  using states::Universal;
  using types::miner::kRewardVestingSpecV1;
  using types::miner::VestSpec;
  using version::NetworkVersion;

  class Monies : public v0::miner::Monies {
   public:
    struct BigFrac {
      BigInt numerator;
      BigInt denominator;
    };

    const BigInt initial_pledge_max_per_byte =
        BigInt(1e18) / BigInt(BigInt(32) << 30);
    const BigFrac initial_pledge_lock_target = {3, 10};

    const int continued_fault_factor_num = 351;
    const int continued_fault_factor_denom = 100;
    const ChainEpoch continued_fault_projection_period =
        ChainEpoch(kEpochsInDay * continued_fault_factor_num
                   / continued_fault_factor_denom);
    const ChainEpoch termination_penalty_lower_bound_projection_period =
        ChainEpoch(kEpochsInDay * 35 / 10);

    const BigFrac termination_reward_factor = {1, 2};
    const ChainEpoch termination_lifetime_cap = 140;

    const int consensus_fault_factor = 5;

    const BigInt locked_reward_factor_num_v6 = BigInt(75);
    const BigInt locked_reward_factor_denom_v6 = BigInt(100);

    const int64_t expected_leader_per_epoch = 5;  // TODO from network.go

    outcome::result<TokenAmount> expectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const ChainEpoch &projection_duration) override;

    outcome::result<TokenAmount> pledgePenaltyForContinuedFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) override;

    outcome::result<TokenAmount> pledgePenaltyForTerminationLowerBound(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) override;

    outcome::result<TokenAmount> pledgePenaltyForTermination(
        const TokenAmount &day_reward,
        const ChainEpoch &sector_age,
        const TokenAmount &twenty_day_reward_activation,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const FilterEstimate &reward_estimate,
        const TokenAmount &replaced_day_reward,
        const ChainEpoch &replaced_sector_age) override;

    outcome::result<TokenAmount> preCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_power_estimate,
        const StoragePower &sector_power) override;

    outcome::result<TokenAmount> initialPledgeForPower(
        const StoragePower &power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;

    outcome::result<TokenAmount> repayDebtsOrAbort(
        Runtime &runtime, Universal<MinerActorState> miner_state) override;

    outcome::result<TokenAmount> consensusFaultPenalty(
        const TokenAmount &this_epoch_reward) override;

    outcome::result<std::pair<TokenAmount, VestSpec>> lockedRewardFromReward(
        const TokenAmount &reward,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> expectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const ChainEpoch &projection_duration) override;

    outcome::result<TokenAmount> pledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> pledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> pledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        const ChainEpoch &sector_age,
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> preCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power) override;

    outcome::result<TokenAmount> initialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;
  };
  CBOR_NON(Monies);

}  // namespace fc::vm::actor::builtin::v2::miner
