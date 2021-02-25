/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/reward/reward_actor_state.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_calculus.hpp"
#include "vm/actor/builtin/v2/reward/policy.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using common::smoothing::FilterEstimate;
  using primitives::kChainEpochUndefined;
  using v0::reward::initBaselinePower;
  using v0::reward::updateToNextEpochWithReward;

  void RewardActorState::initialize(
      const StoragePower &current_realized_power) {
    effective_network_time = 0;
    effective_baseline_power = kBaselineInitialValueV2;
    this_epoch_reward_smoothed =
        FilterEstimate{.position = v0::reward::kInitialRewardPositionEstimate,
                       .velocity = v0::reward::kInitialRewardVelocityEstimate};
    this_epoch_baseline_power =
        initBaselinePower(kBaselineInitialValueV2, kBaselineExponentV3);
    epoch = kChainEpochUndefined;
    simple_total = kSimpleTotal;
    baseline_total = kBaselineTotal;
    updateToNextEpochWithReward(
        *this, current_realized_power, kBaselineExponentV3);
  }

  TokenAmount RewardActorState::simpleTotal() const {
    return simple_total;
  }

  TokenAmount RewardActorState::baselineTotal() const {
    return baseline_total;
  }

}  // namespace fc::vm::actor::builtin::v2::reward
