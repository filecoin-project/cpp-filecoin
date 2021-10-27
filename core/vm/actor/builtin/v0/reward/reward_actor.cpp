/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/reward/reward_actor.hpp"

#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/types/reward/policy.hpp"
#include "vm/actor/builtin/types/reward/reward_actor_calculus.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using states::RewardActorStatePtr;
  using namespace types::reward;

  /// The expected number of block producers in each epoch.
  constexpr auto kExpectedLeadersPerEpoch{5};

  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));

    RewardActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    state->initialize(params);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  outcome::result<TokenAmount> AwardBlockReward::validateParams(
      Runtime &runtime, const Params &params) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    VALIDATE_ARG(params.penalty >= 0);
    VALIDATE_ARG(params.gas_reward >= 0);
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    if (balance < params.gas_reward) {
      ABORT(VMExitCode::kErrIllegalState);
    }
    VALIDATE_ARG(params.win_count > 0);
    return std::move(balance);
  }

  outcome::result<std::tuple<TokenAmount, TokenAmount>>
  AwardBlockReward::calculateReward(Runtime &runtime,
                                    const Params &params,
                                    const TokenAmount &this_epoch_reward,
                                    const TokenAmount &balance) {
    TokenAmount block_reward =
        bigdiv(this_epoch_reward * params.win_count, kExpectedLeadersPerEpoch);
    TokenAmount total_reward = block_reward + params.gas_reward;
    if (total_reward > balance) {
      total_reward = balance;
      block_reward = total_reward - params.gas_reward;
      VM_ASSERT(block_reward >= 0);
    }
    return std::make_tuple(block_reward, total_reward);
  }

  ACTOR_METHOD_IMPL(AwardBlockReward) {
    OUTCOME_TRY(balance, validateParams(runtime, params));
    CHANGE_ERROR_A(
        miner, runtime.resolveAddress(params.miner), VMExitCode::kErrNotFound);
    OUTCOME_TRY(state, runtime.getActorState<RewardActorStatePtr>());
    OUTCOME_TRY(
        reward,
        calculateReward(runtime, params, state->this_epoch_reward, balance));
    const auto &[block_reward, total_reward] = reward;
    state->total_reward += block_reward;
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
    OUTCOME_TRY(state, runtime.getActorState<RewardActorStatePtr>());
    return Result{
        .this_epoch_reward = state->this_epoch_reward,
        .this_epoch_reward_smoothed = state->this_epoch_reward_smoothed,
        .this_epoch_baseline_power = state->this_epoch_baseline_power};
  }

  outcome::result<void> UpdateNetworkKPI::updateKPI(
      Runtime &runtime, const Params &params, const BigInt &baseline_exponent) {
    OUTCOME_TRY(state, runtime.getActorState<RewardActorStatePtr>());
    const ChainEpoch prev_epoch = state->epoch;
    const ChainEpoch now = runtime.getCurrentEpoch();
    while (state->epoch < now) {
      state->updateToNextEpoch(params, baseline_exponent);
    }

    state->updateToNextEpochWithReward(params, baseline_exponent);
    // only update smoothed estimates after updating reward and epoch
    state->updateSmoothedEstimates(state->epoch - prev_epoch);

    // Lotus gas conformance
    OUTCOME_TRY(runtime.commitState(state));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdateNetworkKPI) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStoragePowerAddress));
    const NetworkVersion network_version = runtime.getNetworkVersion();
    const BigInt baseline_exponent = network_version < NetworkVersion::kVersion3
                                         ? kBaselineExponentV0
                                         : kBaselineExponentV3;
    OUTCOME_TRY(updateKPI(runtime, params, baseline_exponent));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Constructor>(),
      exportMethod<AwardBlockReward>(),
      exportMethod<ThisEpochReward>(),
      exportMethod<UpdateNetworkKPI>(),
  };
}  // namespace fc::vm::actor::builtin::v0::reward
