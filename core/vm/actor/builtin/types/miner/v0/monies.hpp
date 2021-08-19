

#pragma once
#include <vm/actor/builtin/types/miner/policy.hpp>
#include <vm/runtime/runtime.hpp>
#include "vm/actor/builtin/types/miner/monies.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using fc::common::math::kPrecision128;
  using fc::common::smoothing::FilterEstimate;
  using fc::primitives::BigInt;
  using fc::primitives::ChainEpoch;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::actor::builtin::types::miner::VestSpec;
  using fc::vm::version::NetworkVersion;

  class Monies : public types::miner::Monies {
   private:
    BigInt lock_target_factor_num = BigInt(3);
    BigInt lock_target_factor_denom = BigInt(10);

    BigInt space_race_initial_pledge_max_per_byte =
        BigInt(1e18) / (BigInt(32) << 30);

    int declared_fault_factor_num_v0 = 214;
    int declared_fault_factor_num_v3 = 351;
    int declared_fault_factor_denom = 100;
    ChainEpoch declared_fault_projection_period_v0 =
        ChainEpoch(kEpochsInDay * declared_fault_factor_num_v0
                   / declared_fault_factor_denom);
    ChainEpoch declared_fault_projection_period_v3 =
        ChainEpoch(kEpochsInDay * declared_fault_factor_num_v3
                   / declared_fault_factor_denom);

    int undeclared_fault_factor_num_v0 = 50;
    int undeclared_fault_factor_num_v1 = 35;
    int undeclared_fault_factor_denom = 10;

    ChainEpoch undeclared_fault_projection_period_v0 =
        ChainEpoch(kEpochsInDay * undeclared_fault_factor_num_v0
                   / undeclared_fault_factor_denom);
    ChainEpoch undeclared_fault_projection_period_v1 =
        ChainEpoch(kEpochsInDay * undeclared_fault_factor_num_v1
                   / undeclared_fault_factor_denom);

   public:
    virtual outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        const ChainEpoch &sector_age,
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) override;

    virtual outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    virtual outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;

    // override function with default return value
    virtual outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForContinuedFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForTerminationLowerBound(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward,
        const ChainEpoch &sector_age,
        const TokenAmount &twenty_day_reward_activation,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const FilterEstimate &reward_estimate,
        const TokenAmount &replaced_day_reward,
        const ChainEpoch &replaced_sector_age) override;

    virtual outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    virtual outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) override;

    virtual outcome::result<TokenAmount> RepayDebtsOrAbort(
        const runtime::Runtime &rt,
        fc::vm::actor::builtin::states::MinerActorState *st) override;

    virtual outcome::result<TokenAmount> ConsensusFaultPenalty(
        const TokenAmount &this_epoch_reward) override;

    virtual outcome::result<std::pair<TokenAmount, VestSpec *>>
    LockedRewardFromReward(const TokenAmount &reward,
                           const NetworkVersion &nv) override;

    virtual outcome::result<TokenAmount> PledgePenaltyForInvalidWindowPoSt(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    virtual fc::outcome::result<std::pair<TokenAmount, VestSpec *>>
    LockedRewardFromReward(const TokenAmount &reward) override;

    ~Monies() override;
  };
  CBOR_NON(Monies);
}  // namespace fc::vm::actor::builtin::v0::miner
