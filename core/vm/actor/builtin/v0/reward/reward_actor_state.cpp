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
        .this_epoch_baseline_power =
            initBaselinePower(kBaselineInitialValueV0, kBaselineExponentV0),
        .epoch = kChainEpochUndefined,
        .total_mined = {0},
    };
    updateToNextEpochWithReward(
        state, current_realized_power, kBaselineExponentV0);
    return state;
  }

}  // namespace fc::vm::actor::builtin::v0::reward
