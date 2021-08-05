/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"

#include "common/smoothing/alpha_beta_filter.hpp"
#include "vm/actor/builtin/types/reward/reward_actor_calculus.hpp"

namespace fc::vm::actor::builtin::states {
  using namespace types::reward;

  void RewardActorState::updateToNextEpoch(
      const StoragePower &current_realized_power,
      const BigInt &baseline_exponent) {
    ++epoch;
    this_epoch_baseline_power =
        baselinePowerFromPrev(this_epoch_baseline_power, baseline_exponent);
    cumsum_realized +=
        std::min(this_epoch_baseline_power, current_realized_power);
    if (cumsum_realized > cumsum_baseline) {
      ++effective_network_time;
      effective_baseline_power =
          baselinePowerFromPrev(effective_baseline_power, baseline_exponent);
      cumsum_baseline += effective_baseline_power;
    }
  }

  void RewardActorState::updateToNextEpochWithReward(
      const StoragePower &current_realized_power,
      const BigInt &baseline_exponent) {
    const auto prev_reward_theta = computeRTheta(effective_network_time,
                                                 effective_baseline_power,
                                                 cumsum_realized,
                                                 cumsum_baseline);
    updateToNextEpoch(current_realized_power, baseline_exponent);
    const auto current_reward_theta = computeRTheta(effective_network_time,
                                                    effective_baseline_power,
                                                    cumsum_realized,
                                                    cumsum_baseline);
    this_epoch_reward = computeReward(epoch,
                                      prev_reward_theta,
                                      current_reward_theta,
                                      simpleTotal(),
                                      baselineTotal());
  }

  void RewardActorState::updateSmoothedEstimates(const ChainEpoch &delta) {
    this_epoch_reward_smoothed =
        nextEstimate(this_epoch_reward_smoothed, this_epoch_reward, delta);
  }

}  // namespace fc::vm::actor::builtin::states
