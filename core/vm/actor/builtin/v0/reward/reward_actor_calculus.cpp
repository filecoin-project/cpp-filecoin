/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/reward/reward_actor_calculus.hpp"

#include "common/math/math.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using common::math::expneg;
  using common::math::kPrecision128;

  StoragePower initBaselinePower() {
    // Q.0 => Q.256
    const auto baselineInitialValue256 =
        BigInt(kBaselineInitialValueV0 << 2 * kPrecision128);
    // Q.256 / Q.128 => Q.128
    const auto baselineAtMinusOne =
        (baselineInitialValue256 / kBaselineExponentV0);
    // Q.128 => Q.0
    return baselineAtMinusOne >> kPrecision128;
  }

  StoragePower baselinePowerFromPrev(
      const StoragePower &prev_epoch_baseline_power,
      NetworkVersion network_version) {
    // Q.0 * Q.128 => Q.128
    const BigInt this_epoch_baseline_power =
        prev_epoch_baseline_power
        * (network_version < NetworkVersion::kVersion3 ? kBaselineExponentV0
                                                       : kBaselineExponentV3);
    // Q.128 => Q.0
    return this_epoch_baseline_power >> kPrecision128;
  }

  BigInt computeRTheta(ChainEpoch effective_network_time,
                       StoragePower baseline_power_at_effective_network_time,
                       SpaceTime cumsum_realized,
                       SpaceTime cumsum_baseline) {
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

  BigInt computeBaselineSupply(const BigInt &theta) {
    // Q.128
    const BigInt theta_lam = (theta * kLambda) >> kPrecision128;

    const BigInt one_sub = (BigInt{1} << kPrecision128) - expneg(theta_lam);

    // Q.0 * Q.128 => Q.128
    return kBaselineTotal * one_sub;
  }

  TokenAmount computeReward(const ChainEpoch &epoch,
                            const BigInt &prev_theta,
                            const BigInt &curr_theta) {
    // Q.0 * Q.128 => Q.128
    TokenAmount simple_reward = kSimpleTotal * kExpLamSubOne;
    // Q.0 * Q.128 => Q.128
    const BigInt epoch_lam = epoch * kLambda;

    // Q.128 * Q.128 => Q.256
    simple_reward *= expneg(epoch_lam);
    // Q.256 >> 128 => Q.128
    simple_reward >>= kPrecision128;

    // Q.128
    const TokenAmount baseline_reward =
        computeBaselineSupply(curr_theta) - computeBaselineSupply(prev_theta);
    // Q.128
    const TokenAmount reward = simple_reward + baseline_reward;

    // Q.128 => Q.0
    return reward >> kPrecision128;
  }
}  // namespace fc::vm::actor::builtin::v0::reward
