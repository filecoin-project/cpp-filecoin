/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using common::smoothing::FilterEstimate;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;

  /// 2.5057116798121726 EiB
  static const StoragePower kBaselineInitialValueV2{"2888888880000000000"};

  /**
   * These numbers are estimates of the onchain constants. They are good for
   * initializing state in devnets and testing but will not match the on chain
   * values exactly which depend on storage onboarding and upgrade epoch
   * history. They are in units of attoFIL, 10^-18 FIL
   */
  // 330M
  static const TokenAmount kDefaultSimpleTotal = BigInt{330e6} * BigInt{1e18};
  // 770M
  static const TokenAmount kDefaultBaselineTotal = BigInt{770e6} * BigInt{1e18};

  struct State {
    static State construct(const StoragePower &current_realized_power);

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
    TokenAmount total_storage_power_reward;

    /**
     * Simple and Baseline totals are constants used for computing rewards.
     * They are on chain because of a historical fix resetting baseline value
     * in a way that depended on the history leading immediately up to the
     * migration fixing the value. These values can be moved from state back
     * into a code constant in a subsequent upgrade.
     */
    TokenAmount simple_total;
    TokenAmount baseline_total;
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
             total_storage_power_reward,
             simple_total,
             baseline_total)

}  // namespace fc::vm::actor::builtin::v2::reward
