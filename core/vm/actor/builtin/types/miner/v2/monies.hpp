

#pragma once
#include <vm/actor/builtin/types/miner/v0/monies.hpp>
#include "common/outcome.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "primitives/types.hpp"
#include "vm/version/version.hpp"

namespace vm::actor::builtin::v2::miner {
    using fc::common::smoothing::FilterEstimate;
    using fc::primitives::ChainEpoch;
    using fc::primitives::StoragePower;
    using fc::primitives::TokenAmount;
    using fc::vm::version::NetworkVersion;

    class Monies : public vm::actor::builtin::v0::miner::Monies {
    protected:
        fc::primitives::BigInt initial_pledge_max_per_byte =
                fc::primitives::BigInt(1e18) / fc::primitives::BigInt(32 << 30);
        // TODO make BigFrac initial_pledge_lock_target

        int continued_fault_factor_num = 351;
        int continued_fault_factor_denom = 100;
        ChainEpoch continued_fault_projection_period =
                ChainEpoch(fc::kEpochsInDay * continued_fault_factor_num
                           / continued_fault_factor_denom);
        ChainEpoch termination_penalty_lower_bound_projection_period =
                ChainEpoch(fc::kEpochsInDay * 35 / 10);

        // TODO make BigFrac termination_reward_factor
        const int termination_lifetime_cap = 140;

        const int consensus_fault_factor = 5;

        fc::primitives::BigInt locked_reward_factor_num_v6 =
                fc::primitives::BigInt(75);
        fc::primitives::BigInt locked_reward_factor_denom_v6 =
                fc::primitives::BigInt(100);

    private:
        virtual fc::outcome::result<TokenAmount> ExpectedRewardForPower(
                const FilterEstimate &reward_estimate,
                FilterEstimate network_qa_power_estimate,
                const StoragePower &qa_sector_power,
                const ChainEpoch &projection_duration) override;

        virtual fc::outcome::result<TokenAmount> PledgePenaltyForDeclaredFault(
                const FilterEstimate &reward_estimate,
                FilterEstimate network_qa_power_estimate,
                const StoragePower &qa_sector_power,
                const NetworkVersion &network_version) override;

        virtual fc::outcome::result<TokenAmount> PledgePenaltyForUndeclaredFault(
                const FilterEstimate &reward_estimate,
                FilterEstimate network_qa_power_estimate,
                const StoragePower &qa_sector_power,
                const NetworkVersion &network_version) override;

        virtual fc::outcome::result<TokenAmount> PledgePenaltyForTermination(
                const TokenAmount &day_reward_at_activation,
                const TokenAmount &twenty_day_reward_activation,
                ChainEpoch sector_age,
                const FilterEstimate &reward_estimate,
                FilterEstimate network_qa_power_estimate,
                const StoragePower &qa_sector_power,
                const NetworkVersion &network_version) override;

        virtual fc::outcome::result<TokenAmount> PreCommitDepositForPower(
                const FilterEstimate &reward_estimate,
                FilterEstimate network_qa_power_estimate,
                const StoragePower &qa_sector_power) override;

        virtual fc::outcome::result<TokenAmount> InitialPledgeForPower(
                const StoragePower &qa_power,
                const StoragePower &baseline_power,
                const TokenAmount &network_total_pledge,
                const FilterEstimate &reward_estimate,
                FilterEstimate network_qa_power_estimate,
                const TokenAmount &network_circulation_supply_smoothed) override;

        /* virtual fc::outcome::result<TokenAmount> RepayDebtsOrAbort(
            const
            ); // TODO
        */

        virtual fc::outcome::result<TokenAmount> consensus_fault_penalty(
                const TokenAmount &this_epouch_reward);
/*
    virtual fc::outcome::result < std::pair < TokenAmount,
        ? ? ? >> (const TokenAmount &reward, const fc::vm::version &nv);
        */ // TODO
    };

}  // namespace vm::actor::builtin::v2::miner
