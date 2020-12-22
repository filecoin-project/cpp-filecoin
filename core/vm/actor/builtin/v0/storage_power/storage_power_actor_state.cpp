/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"

#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "vm/actor/builtin/v0/storage_power/policy.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using adt::Multimap;
  using common::smoothing::kPrecision;
  using common::smoothing::nextEstimate;
  using primitives::kChainEpochUndefined;

  State State::empty(IpldPtr ipld) {
    State state{
        .total_raw_power = 0,
        .total_raw_commited = {},
        .total_qa_power = 0,
        .total_qa_commited = {},
        .total_pledge = 0,
        .this_epoch_raw_power = {},
        .this_epoch_qa_power = {},
        .this_epoch_pledge = {},
        .this_epoch_qa_power_smoothed =
            {.position = kInitialQAPowerEstimatePosition << kPrecision,
             .velocity = kInitialQAPowerEstimateVelocity << kPrecision},
        .miner_count = 0,
        .num_miners_meeting_min_power = {},
        .cron_event_queue = {},
        .first_cron_epoch = {},
        .last_processed_cron_epoch = kChainEpochUndefined,
        .claims = {},
        .proof_validation_batch = {},
    };
    ipld->load(state);
    return state;
  }

  outcome::result<void> State::addToClaim(const Address &miner,
                                          const StoragePower &raw,
                                          const StoragePower &qa) {
    OUTCOME_TRY(claim_found, claims.tryGet(miner));
    if (!claim_found.has_value()) {
      return VMExitCode::kErrNotFound;
    }
    auto claim{claim_found.value()};

    // TotalBytes always update directly
    total_raw_commited += raw;
    total_qa_commited += qa;

    const auto old_claim{claim};
    claim.raw_power += raw;
    claim.qa_power += qa;

    const auto prev_below{old_claim.qa_power < kConsensusMinerMinPower};
    const auto still_below{claim.qa_power < kConsensusMinerMinPower};

    if (prev_below && !still_below) {
      ++num_miners_meeting_min_power;
      total_raw_power += claim.raw_power;
      total_qa_power += claim.qa_power;
    } else if (!prev_below && still_below) {
      --num_miners_meeting_min_power;
      total_raw_power -= old_claim.raw_power;
      total_qa_power -= old_claim.qa_power;
    } else if (!prev_below && !still_below) {
      total_raw_power += raw;
      total_qa_power += qa;
    }
    VM_ASSERT(claim.raw_power >= 0);
    VM_ASSERT(claim.qa_power >= 0);
    VM_ASSERT(num_miners_meeting_min_power >= 0);
    OUTCOME_TRY(claims.set(miner, claim));

    return outcome::success();
  }

  outcome::result<void> State::addPledgeTotal(const TokenAmount &amount) {
    total_pledge += amount;
    VM_ASSERT(total_pledge >= 0);
    return outcome::success();
  }

  outcome::result<void> State::appendCronEvent(const ChainEpoch &epoch,
                                               const CronEvent &event) {
    if (epoch < first_cron_epoch) {
      first_cron_epoch = epoch;
    }
    return Multimap::append(cron_event_queue, epoch, event);
  }

  void State::updateSmoothedEstimate(int64_t delta) {
    this_epoch_qa_power_smoothed =
        nextEstimate(this_epoch_qa_power_smoothed, this_epoch_qa_power, delta);
  }

  std::tuple<StoragePower, StoragePower> State::getCurrentTotalPower() const {
    if (num_miners_meeting_min_power < kConsensusMinerMinMiners) {
      return std::make_tuple(total_raw_commited, total_qa_commited);
    }
    return std::make_tuple(total_raw_power, total_qa_power);
  }

}  // namespace fc::vm::actor::builtin::v0::storage_power
