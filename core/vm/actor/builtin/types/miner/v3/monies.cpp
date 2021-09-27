/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v3/monies.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using types::miner::kRewardVestingSpecV1;

  outcome::result<TokenAmount> Monies::pledgePenaltyForInvalidWindowPoSt(
      const FilterEstimate &reward_estimate,
      const FilterEstimate &network_power_estimate,
      const StoragePower &sector_power) const {
    OUTCOME_TRY(expected_reward,
                expectedRewardForPower(reward_estimate,
                                       network_power_estimate,
                                       sector_power,
                                       invalid_window_po_st_projection_period));
    return expected_reward
           + static_cast<TokenAmount>(base_penalty_for_disputed_window_po_st);
  }

  outcome::result<std::pair<TokenAmount, VestSpec>>
  Monies::lockedRewardFromReward(const TokenAmount &reward,
                                 const NetworkVersion &default_version) const {
    const BigInt lock_amount =
        reward * bigdiv(locked_reward_factor_num, locked_reward_factor_denom);
    return std::make_pair(lock_amount, kRewardVestingSpecV1);
  }
}  // namespace fc::vm::actor::builtin::v3::miner
