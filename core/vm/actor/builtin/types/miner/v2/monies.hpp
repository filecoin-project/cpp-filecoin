/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "vm/actor/builtin/types/miner/v0/monies.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using fc::common::smoothing::FilterEstimate;
  using fc::primitives::ChainEpoch;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::actor::builtin::types::miner::kRewardVestingSpecV1;
  using fc::vm::actor::builtin::types::miner::VestSpec;
  using fc::vm::runtime::Runtime;
  using fc::vm::version::NetworkVersion;
  class Monies : public v0::miner::Monies {
   public:
    struct BigFrac {
      BigInt numerator;
      BigInt denominator;
    };

    BigInt initial_pledge_max_per_byte =
        BigInt(1e18) / BigInt(BigInt(32) << 30);
    BigFrac initial_pledge_lock_target = {3, 10};

    int continued_fault_factor_num = 351;
    int continued_fault_factor_denom = 100;
    ChainEpoch continued_fault_projection_period =
        ChainEpoch(kEpochsInDay * continued_fault_factor_num
                   / continued_fault_factor_denom);
    ChainEpoch termination_penalty_lower_bound_projection_period =
        ChainEpoch(kEpochsInDay * 35 / 10);

    BigFrac termination_reward_factor = {1, 2};
    ChainEpoch termination_lifetime_cap = 140;

    const int consensus_fault_factor = 5;

    BigInt locked_reward_factor_num_v6 = BigInt(75);
    BigInt locked_reward_factor_denom_v6 = BigInt(100);

    long long int expected_leader_per_epoch = 5;  // TODO from network.go

    outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration) override;

    outcome::result<TokenAmount> PledgePenaltyForContinuedFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    outcome::result<TokenAmount> PledgePenaltyForTerminationLowerBound(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward,
        const ChainEpoch &sector_age,
        const TokenAmount &twenty_day_reward_activation,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const FilterEstimate &reward_estimate,
        const TokenAmount &replaced_day_reward,
        const ChainEpoch &replaced_sector_age) override;

    outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;

    outcome::result<TokenAmount> RepayDebtsOrAbort(
        const Runtime &rt,
        fc::vm::actor::builtin::states::MinerActorState *st) override;

    outcome::result<TokenAmount> ConsensusFaultPenalty(
        const TokenAmount &this_epoch_reward) override;

    outcome::result<std::pair<TokenAmount, VestSpec *>> LockedRewardFromReward(
        const TokenAmount &reward, const NetworkVersion &nv) override;

    // override to default from functions v0

    outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration) override;

    outcome::result<TokenAmount> PledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> PledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        const ChainEpoch &sector_age,
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) override;

    outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;
  };
  CBOR_NON(Monies);

}  // namespace fc::vm::actor::builtin::v2::miner
