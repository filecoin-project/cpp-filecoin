/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_calculus.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using miner::ApplyRewards;
  using v0::reward::kBaselineExponentV3;
  using v0::reward::updateSmoothedEstimates;
  using v0::reward::updateToNextEpoch;
  using v0::reward::updateToNextEpochWithReward;

  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.commitState(State::construct(params)));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AwardBlockReward) {
    OUTCOME_TRY(balance,
                v0::reward::AwardBlockReward::validateParams(runtime, params));
    OUTCOME_TRY(miner, runtime.resolveAddress(params.miner));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(reward,
                v0::reward::AwardBlockReward::calculateReward(
                    runtime, params, state.this_epoch_reward, balance));
    const auto &[block_reward, total_reward] = reward;
    state.total_storage_power_reward += block_reward;
    OUTCOME_TRY(runtime.commitState(state));

    VM_ASSERT(total_reward <= balance);

    const TokenAmount penalty = kPenaltyMultiplier * params.penalty;
    const ApplyRewards::Params reward_params{.reward = total_reward,
                                             .penalty = penalty};
    const auto res =
        runtime.sendM<ApplyRewards>(miner, reward_params, total_reward);
    if (res.has_error()) {
      OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, total_reward));
    }

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ThisEpochReward) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    return Result{
        .this_epoch_reward_smoothed = state.this_epoch_reward_smoothed,
        .this_epoch_baseline_power = state.this_epoch_baseline_power};
  }

  ACTOR_METHOD_IMPL(UpdateNetworkKPI) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStoragePowerAddress));
    OUTCOME_TRY(v0::reward::UpdateNetworkKPI::updateKPI<State>(
        runtime, params, kBaselineExponentV3));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Constructor>(),
      exportMethod<AwardBlockReward>(),
      exportMethod<ThisEpochReward>(),
      exportMethod<UpdateNetworkKPI>(),
  };
}  // namespace fc::vm::actor::builtin::v2::reward
