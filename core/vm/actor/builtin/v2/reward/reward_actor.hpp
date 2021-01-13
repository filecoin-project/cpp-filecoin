/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using common::smoothing::FilterEstimate;
  using primitives::StoragePower;

  /// PenaltyMultiplier is the factor miner penalties are scaled up by
  constexpr uint kPenaltyMultiplier = 3;

  struct Constructor : ActorMethodBase<1> {
    using Params = StoragePower;  // current realized power
    ACTOR_METHOD_DECL();
  };

  /**
   * Awards a reward to a block producer.
   * This method is called only by the system actor, implicitly, as the last
   * message in the evaluation of a block. The system actor thus computes the
   * parameters and attached value.
   *
   * The reward includes two components:
   * - the epoch block reward, computed and paid from the reward actor's
   * balance,
   * - the block gas reward, expected to be transferred to the reward actor with
   * this invocation. The reward is reduced before the residual is credited to
   * the block producer, by:
   * - a penalty amount, provided as a parameter, which is burnt,
   */
  struct AwardBlockReward : ActorMethodBase<2> {
    using Params = v0::reward::AwardBlockReward::Params;
    ACTOR_METHOD_DECL();
  };

  /**
   * The award value used for the current epoch, updated at the end of an epoch
   * through cron tick.  In the case previous epochs were null blocks this is
   * the reward value as calculated at the last non-null epoch.
   */
  struct ThisEpochReward : ActorMethodBase<3> {
    struct Result {
      FilterEstimate this_epoch_reward_smoothed;
      StoragePower this_epoch_baseline_power;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ThisEpochReward::Result,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power)

  /**
   * Called at the end of each epoch by the power actor (in turn by its cron
   * hook). This is only invoked for non-empty tipsets, but catches up any
   * number of null epochs to compute the next epoch reward.
   */
  struct UpdateNetworkKPI : ActorMethodBase<4> {
    using Params = v0::reward::UpdateNetworkKPI::Params;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::reward
