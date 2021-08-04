/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/reward/v0/reward_actor_state.hpp"

#include "common/math/math.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "vm/actor/builtin/types/reward/policy.hpp"
#include "vm/actor/builtin/types/reward/reward_actor_calculus.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using common::smoothing::FilterEstimate;
  using primitives::kChainEpochUndefined;
  using namespace types::reward;

  void RewardActorState::initialize(
      const StoragePower &current_realized_power) {
    effective_network_time = 0;
    effective_baseline_power = kBaselineInitialValueV0;
    this_epoch_reward_smoothed =
        FilterEstimate{.position = kInitialRewardPositionEstimate,
                       .velocity = kInitialRewardVelocityEstimate};
    this_epoch_baseline_power =
        initBaselinePower(kBaselineInitialValueV0, kBaselineExponentV0);
    epoch = kChainEpochUndefined;
    updateToNextEpochWithReward(current_realized_power, kBaselineExponentV0);
  }

  TokenAmount RewardActorState::simpleTotal() const {
    return kSimpleTotal;
  }

  TokenAmount RewardActorState::baselineTotal() const {
    return kBaselineTotal;
  }

}  // namespace fc::vm::actor::builtin::v0::reward
