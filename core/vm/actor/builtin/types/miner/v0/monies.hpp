

#pragma once
#include "common/outcome.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "primitives/types.hpp"
#include "vm/version/version.hpp"

namespace vm::actor::builtin::v0::miner {
  using fc::common::smoothing::FilterEstimate;
  using fc::primitives::ChainEpoch;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::version::NetworkVersion;

  class Monies {
   protected:
    int precommit_deposit_factor = 20;
    int initial_pledge_factor = 20;
    ChainEpoch precommit_deposit_projection_period = ChainEpoch(precommit_deposit_factor) * fc::kEpochsInDay;
    ChainEpoch initial_pledge_projection_period = ChainEpoch(initial_pledge_factor) * fc::kEpochsInDay;

    // TODO: move to private----------------------------------------------------
    fc::primitives::BigInt lock_target_factor_num =
        fc::primitives::BigInt(3);
    fc::primitives::BigInt lock_target_factor_denom =
        fc::primitives::BigInt(10);

    fc::primitives::BigInt space_race_initial_pledge_max_per_byte =
        fc::primitives::BigInt(1e18) / fc::primitives::BigInt((32 << 30));

    int declared_fault_factor_num_v0 = 214;
    int declared_fault_factor_num_v3 = 351;
    int declared_fault_factor_denom = 100;
    ChainEpoch declared_fault_projection_period_v0 =
        ChainEpoch(fc::kEpochsInDay * declared_fault_factor_num_v0
                   / declared_fault_factor_denom);
    ChainEpoch declared_fault_projection_period_v3 =
        ChainEpoch(fc::kEpochsInDay * declared_fault_factor_num_v3
                   / declared_fault_factor_denom);

    int undeclared_fault_factor_num_v0 = 50;
    int undeclared_fault_factor_num_v1 = 35;
    int undeclared_fault_factor_denom = 10;

    ChainEpoch undeclared_fault_projection_period_v0 =
        ChainEpoch(fc::kEpochsInDay * undeclared_fault_factor_num_v0
                   / undeclared_fault_factor_denom);
    ChainEpoch undeclared_fault_projection_period_v1 =
        ChainEpoch(fc::kEpochsInDay * undeclared_fault_factor_num_v1
                   / undeclared_fault_factor_denom);
// TODO-------------------------------------------------------------
    const ChainEpoch termination_lifetime_cap = ChainEpoch(70);

   private:
    virtual fc::outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration);

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version);

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version);

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        ChainEpoch sector_age,
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version);

    virtual fc::outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power);

    virtual fc::outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed);
  };

}  // namespace vm::actor::builtin::v0::miner
