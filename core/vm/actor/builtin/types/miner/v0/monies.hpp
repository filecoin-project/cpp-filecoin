/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vm/actor/builtin/types/miner/policy.hpp>
#include <vm/runtime/runtime.hpp>
#include "vm/actor/builtin/types/miner/monies.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using common::math::kPrecision128;
  using common::smoothing::FilterEstimate;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using states::MinerActorState;
  using states::Universal;
  using types::miner::VestSpec;
  using version::NetworkVersion;

  class Monies : public types::miner::Monies {
   private:
    const BigInt lock_target_factor_num = BigInt(3);
    const BigInt lock_target_factor_denom = BigInt(10);

    const BigInt space_race_initial_pledge_max_per_byte =
                     bigdiv(BigInt("1000000000000000000"), (BigInt{32} << 30));

    constexpr static int declared_fault_factor_num_v0 = 214;
    constexpr static int declared_fault_factor_num_v3 = 351;
    constexpr static int declared_fault_factor_denom = 100;
    const ChainEpoch declared_fault_projection_period_v0 =
        ChainEpoch(kEpochsInDay * declared_fault_factor_num_v0
                   / declared_fault_factor_denom);
    const ChainEpoch declared_fault_projection_period_v3 =
        ChainEpoch(kEpochsInDay * declared_fault_factor_num_v3
                   / declared_fault_factor_denom);

    constexpr static int undeclared_fault_factor_num_v0 = 50;
    constexpr static int undeclared_fault_factor_num_v1 = 35;
    constexpr static int undeclared_fault_factor_denom = 10;

    const ChainEpoch undeclared_fault_projection_period_v0 =
        ChainEpoch(kEpochsInDay * undeclared_fault_factor_num_v0
                   / undeclared_fault_factor_denom);
    const ChainEpoch undeclared_fault_projection_period_v1 =
        ChainEpoch(kEpochsInDay * undeclared_fault_factor_num_v1
                   / undeclared_fault_factor_denom);

   public:
    virtual outcome::result<TokenAmount> expectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const ChainEpoch &projection_duration) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        const ChainEpoch &sector_age,
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power,
        const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> preCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const StoragePower &sector_power) override;

    virtual outcome::result<TokenAmount> initialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const std::shared_ptr<FilterEstimate> &network_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;

    // override function with default return value
    virtual outcome::result<TokenAmount> expectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const ChainEpoch &projection_duration) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForContinuedFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForTerminationLowerBound(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForTermination(
        const TokenAmount &day_reward,
        const ChainEpoch &sector_age,
        const TokenAmount &twenty_day_reward_activation,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power,
        const FilterEstimate &reward_estimate,
        const TokenAmount &replaced_day_reward,
        const ChainEpoch &replaced_sector_age) override;

    virtual outcome::result<TokenAmount> preCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_power_estimate,
        const StoragePower &sector_power) override;

    virtual outcome::result<TokenAmount> initialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;

    virtual outcome::result<TokenAmount> repayDebtsOrAbort(
        runtime::Runtime &runtime,
        Universal<MinerActorState> miner_state) override;

    virtual outcome::result<TokenAmount> consensusFaultPenalty(
        const TokenAmount &this_epoch_reward) override;

    virtual outcome::result<std::pair<TokenAmount, VestSpec>>
    lockedRewardFromReward(const TokenAmount &reward,
                           const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> pledgePenaltyForInvalidWindowPoSt(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) override;

    virtual outcome::result<std::pair<TokenAmount, VestSpec>>
    lockedRewardFromReward(const TokenAmount &reward) override;
  };
  CBOR_NON(Monies);
}  // namespace fc::vm::actor::builtin::v0::miner
