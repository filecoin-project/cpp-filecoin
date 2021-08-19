/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <vm/actor/builtin/states/miner/miner_actor_state.hpp>
#include "common/outcome.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::actor::builtin::types::miner  {
  using fc::common::smoothing::FilterEstimate;
  using fc::primitives::BigInt;
  using fc::primitives::ChainEpoch;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::actor::builtin::types::miner::VestSpec;
  using fc::vm::version::NetworkVersion;

  class Monies {
   public:
    int precommit_deposit_factor = 20;
    int initial_pledge_factor = 20;
    ChainEpoch precommit_deposit_projection_period =
        ChainEpoch(precommit_deposit_factor) * fc::kEpochsInDay;
    ChainEpoch initial_pledge_projection_period =
        ChainEpoch(initial_pledge_factor) * fc::kEpochsInDay;
    ChainEpoch termination_lifetime_cap = ChainEpoch(70);
    // v0
    virtual fc::outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration) = 0;

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForDeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) = 0;

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForUndeclaredFault(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) = 0;

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward_at_activation,
        const TokenAmount &twenty_day_reward_activation,
        const ChainEpoch &sector_age,
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const NetworkVersion &network_version) = 0;

    virtual fc::outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const StoragePower &qa_sector_power) = 0;

    virtual fc::outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        FilterEstimate *network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) = 0;

    // v2
    virtual fc::outcome::result<TokenAmount> ExpectedRewardForPower(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const ChainEpoch &projection_duration) = 0;

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForContinuedFault(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) = 0;

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForTerminationLowerBound(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) = 0;

    virtual fc::outcome::result<TokenAmount> PledgePenaltyForTermination(
        const TokenAmount &day_reward,
        const ChainEpoch &sector_age,
        const TokenAmount &twenty_day_reward_activation,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power,
        const FilterEstimate &reward_estimate,
        const TokenAmount &replaced_day_reward,
        const ChainEpoch &replaced_sector_age) = 0;

    virtual outcome::result<TokenAmount> PreCommitDepositForPower(
        const FilterEstimate &reward_estimate,
        FilterEstimate network_qa_power_estimate,
        const StoragePower &qa_sector_power) = 0;

    virtual outcome::result<TokenAmount> InitialPledgeForPower(
        const StoragePower &qa_power,
        const StoragePower &baseline_power,
        const TokenAmount &network_total_pledge,
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const TokenAmount &network_circulation_supply_smoothed) = 0;

    virtual outcome::result<TokenAmount> RepayDebtsOrAbort(
        const Runtime &rt,
        fc::vm::actor::builtin::states::MinerActorState *st) = 0;

    virtual outcome::result<TokenAmount> ConsensusFaultPenalty(
        const TokenAmount &this_epoch_reward) = 0;

    virtual outcome::result<std::pair<TokenAmount, VestSpec *>>
    LockedRewardFromReward(const TokenAmount &reward,
                           const fc::vm::version::NetworkVersion &nv) = 0;

    // v3
    virtual outcome::result<TokenAmount> PledgePenaltyForInvalidWindowPoSt(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) = 0;

    virtual fc::outcome::result<std::pair<TokenAmount, VestSpec *>>
    LockedRewardFromReward(const TokenAmount &reward) = 0;

    virtual ~Monies() = default;
  };
  CBOR_NON(Monies);
}  // namespace fc::vm::actor::builtin::miner
