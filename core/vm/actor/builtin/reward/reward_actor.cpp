/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/reward/reward_actor.hpp"

#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/builtin/miner/policy.hpp"

namespace fc::vm::actor::builtin::reward {
  constexpr auto kMintingInputFixedPoint{30};
  constexpr auto kMintingOutputFixedPoint{97};
  inline const StoragePower kBaselinePower{1ll << 50};
  constexpr auto kExpectedLeadersPerEpoch{5};

  BigInt taylorSeriesExpansion(BigInt ln, BigInt ld, BigInt t) {
    BigInt nb{-ln * t}, db{ld << kMintingInputFixedPoint};
    BigInt n{-nb}, d{db};
    BigInt res;
    for (auto i{0}; i < 25; ++i) {
      d *= i;
      res += (n << kMintingOutputFixedPoint) / d;
      n *= nb;
      d *= db;
      auto b{0};
      for (auto d2{d}; d2; d2 >>= 1) {
        ++b;
      }
      b = std::max(0, b - kMintingOutputFixedPoint);
      n >>= b;
      d >>= b;
    }
    return res;
  }

  BigInt mintingFunction(BigInt f, BigInt t) {
    return (f
            * taylorSeriesExpansion(
                miner::kEpochDurationSeconds
                    * TokenAmount{"6931471805599453094172321215"},
                6 * miner::kSecondsInYear
                    * TokenAmount{"10000000000000000000000000000"},
                t))
           >> kMintingOutputFixedPoint;
  }

  void computePerEpochReward(State &state, int64_t tickets) {
    using boost::multiprecision::pow;
    auto old_simple{state.simple_supply};
    BigInt e6e18{pow(BigInt{10}, 6 * 18)};
    state.simple_supply = mintingFunction(
        100 * e6e18,
        BigInt{state.reward_epochs_paid} << kMintingInputFixedPoint);
    auto old_baseline{state.baseline_supply};
    state.baseline_supply = mintingFunction(900 * e6e18, state.effective_time);
    state.last_per_epoch_reward =
        std::max<TokenAmount>(0, state.baseline_supply - old_baseline)
        + std::max<TokenAmount>(0, state.simple_supply - old_simple);
  }

  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.commitState(State{}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AwardBlockReward) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    VM_ASSERT(balance >= params.gas_reward);
    VM_ASSERT(params.tickets > 0);
    OUTCOME_TRY(miner, runtime.resolveAddress(params.miner));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    TokenAmount total{params.gas_reward
                      + state.last_per_epoch_reward / kExpectedLeadersPerEpoch};
    auto penalty{std::min(params.penalty, total)};
    TokenAmount payable{total - penalty};
    VM_ASSERT(balance >= payable + penalty);
    OUTCOME_TRY(runtime.sendM<miner::AddLockedFund>(miner, payable, payable));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, penalty));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(LastPerEpochReward) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    return state.last_per_epoch_reward;
  }

  ACTOR_METHOD_IMPL(UpdateNetworkKPI) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStoragePowerAddress));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    ++state.reward_epochs_paid;
    state.realized_power = params;
    state.baseline_power = kBaselinePower;
    state.sum_realized += state.realized_power;
    state.sum_baseline += state.baseline_power;
    state.effective_time = (std::min(state.sum_baseline, state.sum_realized)
                            << kMintingInputFixedPoint)
                           / kBaselinePower;
    computePerEpochReward(state, 1);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Constructor>(),
      exportMethod<AwardBlockReward>(),
      exportMethod<LastPerEpochReward>(),
      exportMethod<UpdateNetworkKPI>(),
  };
}  // namespace fc::vm::actor::builtin::reward
