/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::states {
  using common::smoothing::FilterEstimate;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;

  struct RewardActorState : State {
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
    TokenAmount total_reward;

    /**
     * Simple and Baseline totals are constants used for computing rewards.
     * They are on chain because of a historical fix resetting baseline value
     * in a way that depended on the history leading immediately up to the
     * migration fixing the value. These values can be moved from state back
     * into a code constant in a subsequent upgrade.
     */
    TokenAmount simple_total;
    TokenAmount baseline_total;

    // Methods
    virtual void initialize(const StoragePower &current_realized_power) = 0;
    virtual TokenAmount simpleTotal() const = 0;
    virtual TokenAmount baselineTotal() const = 0;
  };

  using RewardActorStatePtr = std::shared_ptr<RewardActorState>;

}  // namespace fc::vm::actor::builtin::states
