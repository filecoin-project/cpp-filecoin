/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/deadline_info.hpp"

#include "vm/actor/builtin/v0/miner/policy.hpp"

namespace fc::vm::actor::builtin::v0::miner {

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

  DeadlineInfo DeadlineInfo::nextNotElapsed() const {
    return elapsed() ? DeadlineInfo(nextPeriodStart(), index, current_epoch)
                     : *this;
  }

  ChainEpoch DeadlineInfo::nextPeriodStart() const {
    return period_start + kWPoStProvingPeriod;
  }

  bool DeadlineInfo::elapsed() const {
    return current_epoch >= close;
  }

  bool DeadlineInfo::faultCutoffPassed() const {
    return current_epoch >= fault_cutoff;
  }

  bool DeadlineInfo::periodStarted() const {
    return current_epoch >= period_start;
  }

  ChainEpoch DeadlineInfo::periodEnd() const {
    return period_start + kWPoStProvingPeriod - 1;
  }

  ChainEpoch DeadlineInfo::last() const {
    return close - 1;
  }

  DeadlineInfo DeadlineInfo::next() const {
    return index == kWPoStPeriodDeadlines
               ? DeadlineInfo(
                   period_start + kWPoStProvingPeriod, 0, current_epoch)
               : DeadlineInfo(period_start, index + 1, current_epoch);
  }

}  // namespace fc::vm::actor::builtin::v0::miner
