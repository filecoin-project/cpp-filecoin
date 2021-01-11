/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/reward/reward_actor_state.hpp"

#include "vm/actor/builtin/v0/reward/reward_actor_calculus.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using primitives::kChainEpochUndefined;
  using v0::reward::baselinePowerFromPrev;
  using v0::reward::computeReward;
  using v0::reward::computeRTheta;
  using v0::reward::initBaselinePower;
  using v0::reward::kBaselineExponentV3;
  using v0::reward::updateToNextEpochWithReward;

  State::State() {}

  State::State(const StoragePower &current_realized_power) {
    effective_network_time = 0;
    effective_baseline_power = kBaselineInitialValueV2;
    this_epoch_reward_smoothed =
        FilterEstimate{.position = v0::reward::kInitialRewardPositionEstimate,
                       .velocity = v0::reward::kInitialRewardVelocityEstimate};
    this_epoch_baseline_power = initBaselinePower(
        kBaselineInitialValueV2, v0::reward::kBaselineExponentV3);
    epoch = kChainEpochUndefined;
    simple_total = kDefaultSimpleTotal;
    baseline_total = kDefaultBaselineTotal;
    updateToNextEpochWithReward(
        *this, current_realized_power, kBaselineExponentV3);
  }

}  // namespace fc::vm::actor::builtin::v2::reward
