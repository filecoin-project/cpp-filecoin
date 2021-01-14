/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/reward/reward_actor.hpp"

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/miner/policy.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  constexpr auto kMintingInputFixedPoint{30};
  constexpr auto kMintingOutputFixedPoint{97};
  inline const StoragePower kBaselinePower{1ll << 50};
  constexpr auto kExpectedLeadersPerEpoch{5};

  BigInt taylorSeriesExpansion(BigInt ln, BigInt ld, BigInt t) {
    BigInt nb{-ln * t}, db{ld << kMintingInputFixedPoint};
    BigInt n{-nb}, d{db};
    BigInt res;
    for (auto i{1}; i < 25; ++i) {
      d *= i;
      res += bigdiv(n << kMintingOutputFixedPoint, d);
      n *= nb;
      d *= db;
      auto b{0};
      for (auto d2{d}; !d2.is_zero(); d2 >>= 1) {
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

  ACTOR_METHOD_IMPL(Constructor) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.commitState(State{}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AwardBlockReward) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(LastPerEpochReward) {
    return outcome::failure(VMExitCode::kNotImplemented);
  }

  ACTOR_METHOD_IMPL(UpdateNetworkKPI) {
    return VMExitCode::kNotImplemented;
  }

  const ActorExports exports{
      exportMethod<Constructor>(),
      exportMethod<AwardBlockReward>(),
      exportMethod<LastPerEpochReward>(),
      exportMethod<UpdateNetworkKPI>(),
  };
}  // namespace fc::vm::actor::builtin::v0::reward
