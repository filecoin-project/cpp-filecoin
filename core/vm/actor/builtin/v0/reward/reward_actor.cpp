/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/reward/reward_actor.hpp"

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::reward {

  /// The expected number of block producers in each epoch.
  constexpr auto kExpectedLeadersPerEpoch{5};

  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.commitState(State::construct(params)));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AwardBlockReward) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.validateArgument(params.penalty >= 0));
    OUTCOME_TRY(runtime.validateArgument(params.gas_reward >= 0));
    OUTCOME_TRY(runtime.validateArgument(params.win_count > 0));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    if (balance < params.gas_reward) {
      OUTCOME_TRY(runtime.abort(VMExitCode::kErrIllegalState));
    }
    OUTCOME_TRY(miner, runtime.resolveAddress(params.miner));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    TokenAmount block_reward = bigdiv(
        state.this_epoch_reward * params.win_count, kExpectedLeadersPerEpoch);
    TokenAmount total_reward = block_reward + params.gas_reward;
    if (total_reward > balance) {
      total_reward = balance;
      block_reward = total_reward - params.gas_reward;
      VM_ASSERT(block_reward >= 0);
    }
    state.total_mined += block_reward;
    OUTCOME_TRY(runtime.commitState(state));

    // Cap the penalty at the total reward value.
    const TokenAmount penalty = std::min(params.penalty, total_reward);

    // Reduce the payable reward by the penalty.
    const TokenAmount reward_payable = total_reward - penalty;
    VM_ASSERT(reward_payable + penalty <= balance);

    // if this fails, we can assume the miner is responsible and avoid failing
    // here.
    const auto res = runtime.sendM<miner::AddLockedFund>(
        miner, reward_payable, reward_payable);
    if (res.has_error()) {
      OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, reward_payable));
    }

    // Burn the penalty amount
    if (penalty > 0) {
      OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, penalty));
    }

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ThisEpochReward) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    return Result{
        .this_epoch_reward = state.this_epoch_reward,
        .this_epoch_reward_smoothed = state.this_epoch_reward_smoothed,
        .this_epoch_baseline_power = state.this_epoch_baseline_power};
  }

  ACTOR_METHOD_IMPL(UpdateNetworkKPI) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStoragePowerAddress));
    const NetworkVersion network_version = runtime.getNetworkVersion();
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    const ChainEpoch prev_epoch = state.epoch;
    const ChainEpoch now = runtime.getCurrentEpoch();
    while (state.epoch < now) {
      state.updateToNextEpoch(params, network_version);
    }

    state.updateToNextEpochWithReward(params, network_version);
    // only update smoothed estimates after updating reward and epoch
    state.updateSmoothedEstimates(state.epoch - prev_epoch);

    // Lotus gas conformance
    OUTCOME_TRY(runtime.commitState(state));

    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Constructor>(),
      exportMethod<AwardBlockReward>(),
      exportMethod<ThisEpochReward>(),
      exportMethod<UpdateNetworkKPI>(),
  };
}  // namespace fc::vm::actor::builtin::v0::reward
