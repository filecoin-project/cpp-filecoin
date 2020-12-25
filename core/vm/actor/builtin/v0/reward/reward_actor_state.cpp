/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"
#include "common/math/math.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_calculus.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using primitives::kChainEpochUndefined;

  State State::construct(const StoragePower &current_realized_power) {
    State state{
        .cumsum_baseline = {0},
        .cumsum_realized = {0},
        .effective_network_time = 0,
        .effective_baseline_power = kBaselineInitialValueV0,
        .this_epoch_reward = {0},
        .this_epoch_reward_smoothed =
            FilterEstimate{.position = kInitialRewardPositionEstimate,
                           .velocity = kInitialRewardVelocityEstimate},
        .this_epoch_baseline_power = initBaselinePower(),
        .epoch = kChainEpochUndefined,
        .total_mined = {0},
    };
    state.updateToNextEpochWithReward(current_realized_power,
                                      NetworkVersion::kVersion0);
    return state;
  }

  void State::updateToNextEpoch(const StoragePower &current_realized_power,
                                NetworkVersion network_version) {
    ++epoch;
    this_epoch_baseline_power =
        baselinePowerFromPrev(this_epoch_baseline_power, network_version);
    cumsum_realized +=
        std::min(this_epoch_baseline_power, current_realized_power);
    if (cumsum_realized > cumsum_baseline) {
      ++effective_network_time;
      effective_baseline_power =
          baselinePowerFromPrev(effective_baseline_power, network_version);
      cumsum_baseline += effective_baseline_power;
    }
  }

  void State::updateToNextEpochWithReward(
      const StoragePower &current_realized_power,
      NetworkVersion network_version) {
    const auto prev_reward_theta = computeRTheta(effective_network_time,
                                                 effective_baseline_power,
                                                 cumsum_realized,
                                                 cumsum_baseline);
    updateToNextEpoch(current_realized_power, network_version);
    const auto current_reward_theta = computeRTheta(effective_network_time,
                                                    effective_baseline_power,
                                                    cumsum_realized,
                                                    cumsum_baseline);
    this_epoch_reward =
        computeReward(epoch, prev_reward_theta, current_reward_theta);
  }

}  // namespace fc::vm::actor::builtin::v0::reward
