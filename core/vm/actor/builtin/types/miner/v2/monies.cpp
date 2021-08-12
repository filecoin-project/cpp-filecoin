//
// Created by Ruslan Gilvanov  on 11.08.2021.
//

#include "monies.hpp"

namespace vm::actor::builtin::v2::miner {
    fc::outcome::result<fc::primitives::TokenAmount>
    vm::actor::builtin::v2::miner::Monies::ExpectedRewardForPower(
            const fc::common::smoothing::FilterEstimate &reward_estimate,
            fc::common::smoothing::FilterEstimate network_qa_power_estimate,
            const fc::primitives::StoragePower &qa_sector_power,
            const fc::primitives::ChainEpoch &projection_duration) {
        fc::primitives::BigInt network_qa_power_smoothed =
                network_qa_power_estimate.position;
        if (!network_qa_power_smoothed) {
            return reward_estimate.position;
        }
        fc::primitives::BigInt expected_reward_for_proving_period = 0;  // TODO ExploratedCumSumofRatio
        fc::primitives::BigInt br128 =
                qa_sector_power * expected_reward_for_proving_period;
        fc::primitives::BigInt br = br128 >> fc::common::math::kPrecision128;
        return std::max(br, fc::primitives::BigInt(0));
    }

    fc::outcome::result<fc::primitives::TokenAmount>
    vm::actor::builtin::v2::miner::Monies::PledgePenaltyForDeclaredFault(
            const FilterEstimate &reward_estimate,
            FilterEstimate network_qa_power_estimate,
            const StoragePower &qa_sector_power,
            const NetworkVersion &network_version) {
        return ExpectedRewardForPower(reward_estimate,
                                      network_qa_power_estimate,
                                      qa_sector_power,
                                      continued_fault_projection_period);
    }

    fc::outcome::result<TokenAmount>
    vm::actor::builtin::v2::miner::Monies::PledgePenaltyForUndeclaredFault(
            const FilterEstimate &reward_estimate,
            FilterEstimate network_qa_power_estimate,
            const StoragePower &qa_sector_power,
            const NetworkVersion &network_version) {
        return ExpectedRewardForPower(reward_estimate,
                                      network_qa_power_estimate,
                                      qa_sector_power,
                                      termination_penalty_lower_bound_projection_period);
    }

    fc::outcome::result<TokenAmount>
    vm::actor::builtin::v2::miner::Monies::PledgePenaltyForTermination(
            const TokenAmount &day_reward_at_activation,
            const TokenAmount &twenty_day_reward_activation,
            ChainEpoch sector_age,
            const FilterEstimate &reward_estimate,
            FilterEstimate network_qa_power_estimate,
            const StoragePower &qa_sector_power,
            const NetworkVersion &network_version) {
        // TODO
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
            FilterEstimate network_qa_power_estimate,
            const TokenAmount &network_circulation_supply_smoothed) {
        TokenAmount ipBase =
                ExpectedRewardForPower(reward_estimate,
                                       network_qa_power_estimate,
                                       qa_power,
                                       initial_pledge_projection_period)
                        .value();

        fc::primitives::BigInt lock_target_num =
                lock_target_factor_num * network_circulation_supply_smoothed; // TODO: lock_target_factor_num -> initial_pledge_lock_target.numerator
        fc::primitives::BigInt lock_target_denom = lock_target_factor_denom; // TODO:

        auto pledge_share_num = qa_power;
        StoragePower network_qa_power = network_qa_power_estimate.position; // TODO: .Estimate()
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
                space_race_initial_pledge_max_per_byte * qa_power;
        return std::min(nominal_pledge, space_race_pledge_cap);
    }

    fc::outcome::result<TokenAmount> vm::actor::builtin::v2::miner::Monies::consensus_fault_penalty(
            const TokenAmount &this_epouch_reward){

    }

}  // namespace vm::actor::builtin::v2::miner
