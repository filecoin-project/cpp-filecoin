/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/types.hpp"
#include "vm/version.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using common::smoothing::FilterEstimate;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using version::NetworkVersion;

  /// 1EiB
  static const StoragePower kBaselineInitialValueV0 = BigInt(1) << 60;

  /**
   * 36.266260308195979333 FIL
   * https://www.wolframalpha.com/input/?i=IntegerPart%5B330%2C000%2C000+*+%281+-+Exp%5B-Log%5B2%5D+%2F+%286+*+%281+year+%2F+30+seconds%29%29%5D%29+*+10%5E18%5D
   */
  static const BigInt kInitialRewardPositionEstimate{"36266260308195979333"};

  /**
   * -1.0982489*10^-7 FIL per epoch.
   * Change of simple minted tokens between epochs 0 and 1
   * https://www.wolframalpha.com/input/?i=IntegerPart%5B%28Exp%5B-Log%5B2%5D+%2F+%286+*+%281+year+%2F+30+seconds%29%29%5D+-+1%29+*+10%5E18%5D
   */
  static const BigInt kInitialRewardVelocityEstimate{-109897758509};

  /**
   * Baseline exponent for network version 0
   * Floor(e^(ln[1 + 200%] / epochsInYear) * 2^128
   * Q.128 formatted number such that f(epoch) = baseExponent^epoch grows 200%
   * in one year of epochs Calculation here:
   * https://www.wolframalpha.com/input/?i=IntegerPart%5BExp%5BLog%5B1%2B200%25%5D%2F%28%28365+days%29%2F%2830+seconds%29%29%5D*2%5E128%5D
   */
  static const BigInt kBaselineExponentV0{
      "340282722551251692435795578557183609728"};

  /**
   * Baseline exponent for network version 3
   * Floor(e^(ln[1 + 100%] / epochsInYear) * 2^128
   * Q.128 formatted number such that f(epoch) = baseExponent^epoch grows 100%
   * in one year of epochs
   * Calculation here:
   * https://www.wolframalpha.com/input/?i=IntegerPart%5BExp%5BLog%5B1%2B100%25%5D%2F%28%28365+days%29%2F%2830+seconds%29%29%5D*2%5E128%5D
   */
  static const BigInt kBaselineExponentV3{
      "340282591298641078465964189926313473653"};

  struct State {
    State() = default;
    explicit State(const StoragePower &current_realized_power);

    /**
     * Target CumsumRealized needs to reach for EffectiveNetworkTime to
     * increase. Expressed in byte-epochs.
     */
    SpaceTime cumsum_baseline;

    /**
     * Cumulative sum of network power capped by BalinePower(epoch).
     * Expressed in byte-epochs.
     */
    SpaceTime cumsum_realized;

    /**
     * Ceiling of real effective network time `theta` based on
     * CumsumBaselinePower(theta) == CumsumRealizedPower Theta captures the
     * notion of how much the network has progressed in its baseline and in
     * advancing network time.
     */
    ChainEpoch effective_network_time;

    /**
     * Baseline power at the EffectiveNetworkTime epoch
     */
    StoragePower effective_baseline_power;

    /**
     * The reward to be paid in per WinCount to block producers.
     * The actual reward total paid out depends on the number of winners in any
     * round. This value is recomputed every non-null epoch and used in the next
     * non-null epoch.
     */
    TokenAmount this_epoch_reward;

    /**
     * Smoothed this_epoch_reward
     */
    FilterEstimate this_epoch_reward_smoothed;

    /**
     * The baseline power the network is targeting at st.Epoch
     */
    StoragePower this_epoch_baseline_power;

    /**
     * Epoch tracks for which epoch the Reward was computed
     */
    ChainEpoch epoch;

    /**
     * Tracks the total FIL awarded to block miners
     */
    TokenAmount total_mined;
  };
  CBOR_TUPLE(State,
             cumsum_baseline,
             cumsum_realized,
             effective_network_time,
             effective_baseline_power,
             this_epoch_reward,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power,
             epoch,
             total_mined)

}  // namespace fc::vm::actor::builtin::v0::reward
