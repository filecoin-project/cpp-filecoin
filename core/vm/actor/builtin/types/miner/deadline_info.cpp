/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_info.hpp"

#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {

  DeadlineInfo::DeadlineInfo(ChainEpoch start,
                             size_t deadline_index,
                             ChainEpoch now) {
    if (deadline_index < kWPoStPeriodDeadlines) {
      const auto deadline_open{start
                               + static_cast<ChainEpoch>(deadline_index)
                                     * kWPoStChallengeWindow};
      current_epoch = now;
      period_start = start;
      index = deadline_index;
      open = deadline_open;
      close = open + kWPoStChallengeWindow;
      challenge = open - kWPoStChallengeLookback;
      fault_cutoff = open - kFaultDeclarationCutoff;
    } else {
      // Return deadline info for a no-duration deadline immediately after the
      // last real one
      const ChainEpoch after_last_deadline{start + kWPoStProvingPeriod};
      current_epoch = now;
      period_start = start;
      index = deadline_index;
      open = after_last_deadline;
      close = after_last_deadline;
      challenge = after_last_deadline;
      fault_cutoff = 0;
    }
    wpost_period_deadlines = kWPoStPeriodDeadlines;
    wpost_proving_period = kWPoStProvingPeriod;
    wpost_challenge_window = kWPoStChallengeWindow;
    wpost_challenge_lookback = kWPoStChallengeLookback;
    fault_declaration_cutoff = kFaultDeclarationCutoff;
  }

  bool DeadlineInfo::periodStarted() const {
    return current_epoch >= period_start;
  }

  bool DeadlineInfo::periodElapsed() const {
    return current_epoch >= nextPeriodStart();
  }

  ChainEpoch DeadlineInfo::periodEnd() const {
    return period_start + kWPoStProvingPeriod - 1;
  }

  ChainEpoch DeadlineInfo::nextPeriodStart() const {
    return period_start + kWPoStProvingPeriod;
  }

  bool DeadlineInfo::isOpen() const {
    return (current_epoch >= open) && (current_epoch < close);
  }

  bool DeadlineInfo::hasElapsed() const {
    return current_epoch >= close;
  }

  ChainEpoch DeadlineInfo::last() const {
    return close - 1;
  }

  ChainEpoch DeadlineInfo::nextOpen() const {
    return close;
  }

  bool DeadlineInfo::faultCutoffPassed() const {
    return current_epoch >= fault_cutoff;
  }

  DeadlineInfo DeadlineInfo::nextNotElapsed() const {
    if (hasElapsed()) {
      return *this;
    }

    auto offset = period_start % wpost_proving_period;
    if (offset < 0) {
      offset += wpost_proving_period;
    }

    const auto global_period = current_epoch / wpost_proving_period;
    ChainEpoch start_period = global_period * wpost_proving_period + offset;

    while (start_period > current_epoch) {
      start_period -= wpost_proving_period;
    }

    if (current_epoch
        >= ChainEpoch(start_period + (index + 1) * kWPoStChallengeWindow)) {
      start_period += wpost_proving_period;
    }

    return DeadlineInfo(start_period, index, current_epoch);
  }

  QuantSpec DeadlineInfo::quant() const {
    return QuantSpec(kWPoStProvingPeriod, last());
  }

}  // namespace fc::vm::actor::builtin::types::miner
