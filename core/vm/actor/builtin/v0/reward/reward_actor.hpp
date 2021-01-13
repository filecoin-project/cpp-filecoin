/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_calculus.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;

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
    struct Params {
      Address miner;
      /// penalty for including bad messages in a block, >= 0
      TokenAmount penalty;
      /// gas reward from all gas fees in a block, >= 0
      TokenAmount gas_reward;
      /// number of reward units won, > 0
      int64_t win_count;
    };

    /**
     * Validates input parameters and returns current balance
     * Logic present in v0 and v2 actor can be reused with this function.
     * @param runtime
     * @param params
     * @return balance - current actor balance
     */
    static outcome::result<TokenAmount> validateParams(Runtime &runtime,
                                                       const Params &params);

    /**
     * Calculates
     * Logic present in v0 and v2 actor can be reused with this function
     * @param runtime
     * @param params
     * @param this_epoch_reward
     * @param balance
     * @return block reward and total reward tuple
     */
    static outcome::result<std::tuple<TokenAmount, TokenAmount>>
    calculateReward(Runtime &runtime,
                    const Params &params,
                    const TokenAmount &this_epoch_reward,
                    const TokenAmount &balance);

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AwardBlockReward::Params, miner, penalty, gas_reward, win_count)

  /**
   * The award value used for the current epoch, updated at the end of an epoch
   * through cron tick.  In the case previous epochs were null blocks this is
   * the reward value as calculated at the last non-null epoch.
   */
  struct ThisEpochReward : ActorMethodBase<3> {
    struct Result {
      TokenAmount this_epoch_reward;
      FilterEstimate this_epoch_reward_smoothed;
      StoragePower this_epoch_baseline_power;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ThisEpochReward::Result,
             this_epoch_reward,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power)

  /**
   * Called at the end of each epoch by the power actor (in turn by its cron
   * hook). This is only invoked for non-empty tipsets, but catches up any
   * number of null epochs to compute the next epoch reward.
   */
  struct UpdateNetworkKPI : ActorMethodBase<4> {
    using Params = StoragePower;

    /**
     * Updates network KPI
     * Logic present in v0 and v2 actor can be reused with this function.
     * @tparam TState - Reward Actor state
     * @param runtime
     * @param params method
     * @param baseline_exponent - constant
     */
    template <class TState>
    static outcome::result<void> updateKPI(Runtime &runtime,
                                           const Params &params,
                                           const BigInt &baseline_exponent) {
      OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<TState>());
      const ChainEpoch prev_epoch = state.epoch;
      const ChainEpoch now = runtime.getCurrentEpoch();
      while (state.epoch < now) {
        updateToNextEpoch(state, params, baseline_exponent);
      }

      updateToNextEpochWithReward(state, params, baseline_exponent);
      // only update smoothed estimates after updating reward and epoch
      updateSmoothedEstimates(state, state.epoch - prev_epoch);

      // Lotus gas conformance
      OUTCOME_TRY(runtime.commitState(state));

      return outcome::success();
    }

    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::reward
