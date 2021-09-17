/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_info.hpp"

#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {

  DeadlineInfo::DeadlineInfo(ChainEpoch start,
                             uint64_t deadline_index,
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
    if (!hasElapsed()) {
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

  bool DeadlineInfo::operator==(const DeadlineInfo &other) const {
    return current_epoch == other.current_epoch
           && period_start == other.period_start && index == other.index
           && open == other.open && close == other.close
           && challenge == other.challenge && fault_cutoff == other.fault_cutoff
           && wpost_period_deadlines == other.wpost_period_deadlines
           && wpost_proving_period == other.wpost_proving_period
           && wpost_challenge_window == other.wpost_challenge_window
           && wpost_challenge_lookback == other.wpost_challenge_lookback
           && fault_declaration_cutoff == other.fault_declaration_cutoff;
  }

  bool deadlineIsMutable(ChainEpoch proving_period_start,
                         uint64_t dl_id,
                         ChainEpoch curr_epoch) {
    // Get the next non-elapsed deadline (i.e., the next time we care about
    // mutations to the deadline).
    const DeadlineInfo dl_info =
        DeadlineInfo(proving_period_start, dl_id, curr_epoch).nextNotElapsed();

    // Ensure that the current epoch is at least one challenge window before
    // that deadline opens.
    return curr_epoch < dl_info.open - kWPoStChallengeWindow;
  }

  bool deadlineAvailableForOptimisticPoStDispute(
      ChainEpoch proving_period_start, uint64_t dl_id, ChainEpoch curr_epoch) {
    if (proving_period_start > curr_epoch) {
      // We haven't started proving yet, there's nothing to dispute.
      return false;
    }

    const DeadlineInfo dl_info =
        DeadlineInfo(proving_period_start, dl_id, curr_epoch).nextNotElapsed();

    return !dl_info.isOpen()
           && curr_epoch
                  < (dl_info.close - kWPoStProvingPeriod) + kWPoStDisputeWindow;
  }

  bool deadlineAvailableForCompaction(ChainEpoch proving_period_start,
                                      uint64_t dl_id,
                                      ChainEpoch curr_epoch) {
    return deadlineIsMutable(proving_period_start, dl_id, curr_epoch)
           && !deadlineAvailableForOptimisticPoStDispute(
               proving_period_start, dl_id, curr_epoch);
  }

  DeadlineInfo newDeadlineInfoFromOffsetAndEpoch(ChainEpoch period_start_seed,
                                                 ChainEpoch curr_epoch) {
    const auto quant = QuantSpec(kWPoStProvingPeriod, period_start_seed);
    const auto current_period_start = quant.quantizeDown(curr_epoch);
    const uint64_t current_deadline_index =
        ((curr_epoch - current_period_start) / kWPoStChallengeWindow)
        % kWPoStPeriodDeadlines;
    return DeadlineInfo(
        current_period_start, current_deadline_index, curr_epoch);
  }

}  // namespace fc::vm::actor::builtin::types::miner
