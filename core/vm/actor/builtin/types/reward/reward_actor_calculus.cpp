/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/reward/reward_actor_calculus.hpp"

#include "common/math/math.hpp"

namespace fc::vm::actor::builtin::types::reward {
  using common::math::expneg;
  using common::math::kLambda;
  using common::math::kPrecision128;

  StoragePower initBaselinePower(const BigInt &initial_value,
                                 const BigInt &baseline_exponent) {
    // Q.0 => Q.256
    const auto baselineInitialValue256 =
        BigInt(initial_value << 2 * kPrecision128);
    // Q.256 / Q.128 => Q.128
    const auto baselineAtMinusOne =
        (baselineInitialValue256 / baseline_exponent);
    // Q.128 => Q.0
    return baselineAtMinusOne >> kPrecision128;
  }

  StoragePower baselinePowerFromPrev(
      const StoragePower &prev_epoch_baseline_power,
      const BigInt &baseline_exponent) {
    // Q.0 * Q.128 => Q.128
    const BigInt this_epoch_baseline_power =
        prev_epoch_baseline_power * baseline_exponent;
    // Q.128 => Q.0
    return this_epoch_baseline_power >> kPrecision128;
  }

  BigInt computeRTheta(
      const ChainEpoch &effective_network_time,
      const StoragePower &baseline_power_at_effective_network_time,
      const SpaceTime &cumsum_realized,
      const SpaceTime &cumsum_baseline) {
    BigInt reward_theta{0};
    if (effective_network_time != 0) {
      // Q.128
      reward_theta = BigInt{effective_network_time} << kPrecision128;
      BigInt diff = cumsum_baseline - cumsum_realized;
      // Q.0 => Q.128
      diff <<= kPrecision128;
      diff = bigdiv(diff, baseline_power_at_effective_network_time);
      // Q.128
      reward_theta -= diff;
    }
    return reward_theta;
  }

  BigInt computeBaselineSupply(const BigInt &theta,
                               const BigInt &baseline_total) {
    // Q.128
    const BigInt theta_lam = (theta * kLambda) >> kPrecision128;

    const BigInt one_sub = (BigInt{1} << kPrecision128) - expneg(theta_lam);

    // Q.0 * Q.128 => Q.128
    return baseline_total * one_sub;
  }

  TokenAmount computeReward(const ChainEpoch &epoch,
                            const BigInt &prev_theta,
                            const BigInt &curr_theta,
                            const BigInt &simple_total,
                            const BigInt &baseline_total) {
    // Q.0 * Q.128 => Q.128
    TokenAmount simple_reward = simple_total * kExpLamSubOne;
    // Q.0 * Q.128 => Q.128
    const BigInt epoch_lam = epoch * kLambda;

    // Q.128 * Q.128 => Q.256
    simple_reward *= expneg(epoch_lam);
    // Q.256 >> 128 => Q.128
    simple_reward >>= kPrecision128;

    // Q.128
    const TokenAmount baseline_reward =
        computeBaselineSupply(curr_theta, baseline_total)
        - computeBaselineSupply(prev_theta, baseline_total);
    // Q.128
    const TokenAmount reward = simple_reward + baseline_reward;

    // Q.128 => Q.0
    return reward >> kPrecision128;
  }
}  // namespace fc::vm::actor::builtin::types::reward
