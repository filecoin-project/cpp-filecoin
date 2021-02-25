/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"
#include "vm/actor/builtin/states/reward_actor_state.hpp"
#include "vm/version.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using version::NetworkVersion;

  /**
   * expLamSubOne = e^lambda - 1
   * for Q.128: int(expLamSubOne * 2^128)
   * Calculation here:
   * https://www.wolframalpha.com/input/?i=IntegerPart%5B%5BExp%5BLog%5B2%5D+%2F+%286+*+%281+year+%2F+30+seconds%29%29%5D+-+1%5D+*+2%5E128%5D
   **/
  static const BigInt kExpLamSubOne{"37396273494747879394193016954629"};

  /**
   * Initialize baseline power for epoch -1 so that baseline power at epoch 0 is
   * kBaselineInitialValue
   */
  StoragePower initBaselinePower(const BigInt &initial_value,
                                 const BigInt &baseline_exponent);

  /**
   * Compute BaselinePower(t) from BaselinePower(t-1) with an additional
   * multiplication of the base exponent
   * @return new baseline power
   */
  StoragePower baselinePowerFromPrev(
      const StoragePower &prev_epoch_baseline_power,
      const BigInt &baseline_exponent);

  /**
   * Computes RewardTheta which is is precise fractional value of
   * effectiveNetworkTime. The effectiveNetworkTime is defined by
   * CumsumBaselinePower(theta) == CumsumRealizedPower. As baseline power is
   * defined over integers and the RewardTheta is required to be fractional, we
   * perform linear interpolation between CumsumBaseline(⌊theta⌋) and
   * CumsumBaseline(⌈theta⌉). The effectiveNetworkTime argument is ceiling of
   * theta. The result is a fractional effectiveNetworkTime (theta) in Q.128
   * format.
   */
  BigInt computeRTheta(
      const ChainEpoch &effective_network_time,
      const StoragePower &baseline_power_at_effective_network_time,
      const SpaceTime &cumsum_realized,
      const SpaceTime &cumsum_baseline);

  /**
   * Computes baseline supply based on theta in Q.128 format
   * @returns baseline supply in Q.128 format
   */
  BigInt computeBaselineSupply(const BigInt &theta,
                               const BigInt &baseline_total);

  /**
   * Computes a reward for all expected leaders when effective network time
   * changes from prevTheta to currTheta
   * Inputs are in Q.128 format
   * @param abi
   */
  TokenAmount computeReward(const ChainEpoch &epoch,
                            const BigInt &prev_theta,
                            const BigInt &curr_theta,
                            const BigInt &simple_total,
                            const BigInt &baseline_total);

  /**
   * Used for update of internal state during null rounds
   *
   * @tparam State - Reward Actor State
   * @param state to update
   * @param current_realized_power
   * @param baseline_exponent - depends on actor version
   */
  void updateToNextEpoch(states::RewardActorState &state,
                         const StoragePower &current_realized_power,
                         const BigInt &baseline_exponent);

  /**
   * Updates reward state to track reward for the next epoch
   * @tparam State - Reward Actor State
   * @param state to update
   * @param current_realized_power
   */
  void updateToNextEpochWithReward(states::RewardActorState &state,
                                   const StoragePower &current_realized_power,
                                   const BigInt &baseline_exponent);

  /**
   * Update smoothed estimate for state
   * @tparam State - Reward Actor State
   * @param state to update
   * @param delta
   */
  void updateSmoothedEstimates(states::RewardActorState &state,
                               const ChainEpoch &delta);

}  // namespace fc::vm::actor::builtin::v0::reward
