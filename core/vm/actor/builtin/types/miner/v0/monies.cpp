//
// Created by Ruslan Gilvanov  on 11.08.2021.
//

#include "monies.hpp"

namespace fc::vm::actor::builtin::v0::miner {

  outcome::result<TokenAmount> Monies::ExpectedRewardForPower(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const ChainEpoch &projection_duration) {
    BigInt network_qa_power_smoothed = estimate(*network_qa_power_estimate);
    if (!network_qa_power_smoothed) {
      return estimate(reward_estimate);
    }
    BigInt expected_reward_for_proving_period = extrapolatedCumSumOfRatio(
        projection_duration, 0, reward_estimate, *network_qa_power_estimate);
    BigInt br = qa_sector_power * expected_reward_for_proving_period;
    return br >> kPrecision128;
  }

  outcome::result<fc::primitives::TokenAmount>
  Monies::PledgePenaltyForDeclaredFault(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const NetworkVersion &network_version) {
    ChainEpoch projection_period = declared_fault_projection_period_v0;
    if (network_version >= NetworkVersion::kVersion3) {
      projection_period = declared_fault_factor_num_v3;
    }
    return ExpectedRewardForPower(reward_estimate,
                                  network_qa_power_estimate,
                                  qa_sector_power,
                                  projection_period);
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForUndeclaredFault(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const NetworkVersion &network_version) {
    ChainEpoch projection_period = undeclared_fault_projection_period_v0;
    if (network_version >= NetworkVersion::kVersion1) {
      projection_period = undeclared_fault_projection_period_v1;
    }
    return ExpectedRewardForPower(reward_estimate,
                                  network_qa_power_estimate,
                                  qa_sector_power,
                                  projection_period);
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForTermination(
      const TokenAmount &day_reward_at_activation,
      const TokenAmount &twenty_day_reward_activation,
      const ChainEpoch &sector_age,
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const NetworkVersion &network_version) {
    BigInt capped_sector_age = BigInt(std::min(
        BigInt(sector_age), BigInt(termination_lifetime_cap * kEpochsInDay)));
    if (network_version >= NetworkVersion::kVersion1) {
      capped_sector_age =
          BigInt(std::min(BigInt(sector_age / 2),
                          BigInt(termination_lifetime_cap * kEpochsInDay)));
    }

    return std::max(
        PledgePenaltyForUndeclaredFault(reward_estimate,
                                        network_qa_power_estimate,
                                        qa_sector_power,
                                        network_version)
            .value(),
        BigInt(twenty_day_reward_activation
               + BigInt(day_reward_at_activation * capped_sector_age)
                     / BigInt(kEpochsInDay)));
  }

  outcome::result<TokenAmount> Monies::PreCommitDepositForPower(
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return ExpectedRewardForPower(reward_estimate,
                                  network_qa_power_estimate,
                                  qa_sector_power,
                                  precommit_deposit_projection_period);
  }

  outcome::result<TokenAmount> Monies::InitialPledgeForPower(
      const StoragePower &qa_power,
      const StoragePower &baseline_power,
      const TokenAmount &network_total_pledge,
      const FilterEstimate &reward_estimate,
      FilterEstimate *network_qa_power_estimate,
      const TokenAmount &network_circulation_supply_smoothed) {
    StoragePower network_qa_power = estimate(*network_qa_power_estimate);
    TokenAmount ipBase =
        ExpectedRewardForPower(reward_estimate,
                               network_qa_power_estimate,
                               qa_power,
                               initial_pledge_projection_period)
            .value();

    BigInt lock_target_num =
        lock_target_factor_num * network_circulation_supply_smoothed;
    BigInt lock_target_denom = lock_target_factor_denom;

    StoragePower pledge_share_num = qa_power;
    BigInt pledge_share_denom =
        std::max(std::max(network_qa_power, baseline_power), qa_power);
    BigInt additional_ip_num = lock_target_num * pledge_share_num;
    BigInt additional_ip_denom = lock_target_denom * pledge_share_denom;
    BigInt additional_ip = additional_ip_num / additional_ip_denom;

    BigInt nominal_pledge = ipBase + additional_ip;
    BigInt space_race_pledge_cap =
        space_race_initial_pledge_max_per_byte * qa_power;
    return std::min(nominal_pledge, space_race_pledge_cap);
  }
  // override function with default return value

  outcome::result<TokenAmount> Monies::ExpectedRewardForPower(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const ChainEpoch &projection_duration) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForContinuedFault(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForTerminationLowerBound(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PledgePenaltyForTermination(
      const TokenAmount &day_reward,
      const ChainEpoch &sector_age,
      const TokenAmount &twenty_day_reward_activation,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power,
      const FilterEstimate &reward_estimate,
      const TokenAmount &replaced_day_reward,
      const ChainEpoch &replaced_sector_age) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::PreCommitDepositForPower(
      const FilterEstimate &reward_estimate,
      FilterEstimate network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::InitialPledgeForPower(
      const StoragePower &qa_power,
      const StoragePower &baseline_power,
      const TokenAmount &network_total_pledge,
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const TokenAmount &network_circulation_supply_smoothed) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::RepayDebtsOrAbort(
      const runtime::Runtime &rt,
      fc::vm::actor::builtin::states::MinerActorState *st) {
    return TokenAmount{};
  }

  outcome::result<TokenAmount> Monies::ConsensusFaultPenalty(
      const TokenAmount &this_epoch_reward) {
    return TokenAmount{};
  }

  outcome::result<std::pair<TokenAmount, VestSpec *>>
  Monies::LockedRewardFromReward(const TokenAmount &reward,
                                 const NetworkVersion &nv) {
    return std::pair(TokenAmount{}, nullptr);
  }

  fc::outcome::result<TokenAmount> Monies::PledgePenaltyForInvalidWindowPoSt(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power){
    return TokenAmount{};
  }

  fc::outcome::result<std::pair<TokenAmount, VestSpec *>>
  Monies::LockedRewardFromReward(const TokenAmount &reward){
    return {TokenAmount{}, new VestSpec()};
  }

  Monies::~Monies(){}
}  // namespace fc::vm::actor::builtin::v0::miner
