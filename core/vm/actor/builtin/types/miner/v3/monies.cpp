/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "monies.hpp"

namespace fc::vm::actor::builtin::v3::miner {

  outcome::result<TokenAmount>
  Monies::PledgePenaltyForInvalidWindowPoSt(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_qa_power_estimate,
      const StoragePower &qa_sector_power) {
    OUTCOME_TRY(expected_reward, ExpectedRewardForPower(reward_estimate,
                           network_qa_power_estimate,
                           qa_sector_power,
                           invalid_window_po_st_projection_period));
    return expected_reward
           + static_cast<TokenAmount>(base_penalty_for_disputed_window_po_st);
  }

  outcome::result<std::pair<fc::primitives::TokenAmount, VestSpec *>>
  Monies::LockedRewardFromReward(
      const TokenAmount &reward) {
    BigInt lock_amount =
        reward * locked_reward_factor_num / locked_reward_factor_denom;
    return {lock_amount,
            const_cast<VestSpec *>(
                &types::miner::
                    kRewardVestingSpecV1)};
  }
}  // namespace vm::actor::builtin::v3::miner
