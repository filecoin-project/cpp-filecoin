/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;

  /**
   * Deadline calculations with respect to a current epoch.
   * "Deadline" refers to the window during which proofs may be submitted.
   * Windows are non-overlapping ranges [Open, Close), but the challenge epoch
   * for a window occurs before the window opens. The current epoch may not
   * necessarily lie within the deadline or proving period represented here
   */
  struct DeadlineInfo {
    DeadlineInfo() = default;

    DeadlineInfo(ChainEpoch start, uint64_t deadline_index, ChainEpoch now);

    /** Whether the proving period has begun. */
    bool periodStarted() const;

    /** Whether the proving period has elapsed. */
    bool periodElapsed() const;

    /** The last epoch in the proving period. */
    ChainEpoch periodEnd() const;

    /** The first epoch in the next proving period. */
    ChainEpoch nextPeriodStart() const;

    /** Whether the current deadline is currently open. */
    bool isOpen() const;

    /** Whether the current deadline has already closed. */
    bool hasElapsed() const;

    /** The last epoch during which a proof may be submitted. */
    ChainEpoch last() const;

    /** Epoch at which the subsequent deadline opens. */
    ChainEpoch nextOpen() const;

    /** Whether the deadline's fault cutoff has passed. */
    bool faultCutoffPassed() const;

    /** Returns the next instance of this deadline that has not yet elapsed. */
    DeadlineInfo nextNotElapsed() const;

    /** Returns QuantSpec for Deadline Info. */
    QuantSpec quant() const;

    bool operator==(const DeadlineInfo &other) const;

    bool operator!=(const DeadlineInfo &other) const {
      return !(*this == other);
    }

    /// Deadline parameters

    /** Epoch at which this info was calculated. */
    ChainEpoch current_epoch{};

    /** First epoch of the proving period (<= CurrentEpoch). */
    ChainEpoch period_start{};

    /**
     * A deadline index, in [0..d.WPoStProvingPeriodDeadlines) unless period
     * elapsed.
     */
    uint64_t index{};

    /** First epoch from which a proof may be submitted (>= CurrentEpoch). */
    ChainEpoch open{};

    /** First epoch from which a proof may no longer be submitted (>= Open). */
    ChainEpoch close{};

    /** Epoch at which to sample the chain for challenge (< Open). */
    ChainEpoch challenge{};

    /** First epoch at which a fault declaration is rejected (< Open). */
    ChainEpoch fault_cutoff{};

    /// Protocol parameters
    uint64_t wpost_period_deadlines{};

    /** The number of epochs in a window post proving period. */
    ChainEpoch wpost_proving_period{};

    ChainEpoch wpost_challenge_window{};

    ChainEpoch wpost_challenge_lookback{};

    ChainEpoch fault_declaration_cutoff{};
  };

  /**
   * Determine current period start and deadline index directly from current
   * epoch and the offset implied by the proving period. This works correctly
   * even for the state of a miner actor without an active deadline cron
   */
  DeadlineInfo newDeadlineInfoFromOffsetAndEpoch(ChainEpoch period_start_seed,
                                                 ChainEpoch curr_epoch);

}  // namespace fc::vm::actor::builtin::types::miner
