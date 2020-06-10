/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "vm/actor/builtin/storage_power/policy.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::storage_power {
  using adt::Multimap;

  State State::empty(IpldPtr ipld) {
    State state{
        .total_raw_power = 0,
        .total_qa_power = 0,
        .total_pledge = 0,
        .miner_count = 0,
        .cron_event_queue = {},
        .last_epoch_tick = 0,
        .claims = {},
        .num_miners_meeting_min_power = 0,
    };
    ipld->load(state);
    return state;
  }

  outcome::result<void> State::addToClaim(const Address &miner,
                                          const StoragePower &raw,
                                          const StoragePower &qa) {
    OUTCOME_TRY(claim, claims.get(miner));
    auto old_nominal{claim.qa_power};
    claim.raw_power += raw;
    claim.qa_power += qa;
    auto new_nominal{claim.qa_power};
    auto prev_below{old_nominal < kConsensusMinerMinPower};
    auto still_below{new_nominal < kConsensusMinerMinPower};
    if (prev_below && !still_below) {
      ++num_miners_meeting_min_power;
      total_raw_power += claim.raw_power;
      total_qa_power += new_nominal;
    } else if (!prev_below && still_below) {
      --num_miners_meeting_min_power;
      total_raw_power -= claim.raw_power;
      total_qa_power -= old_nominal;
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

  outcome::result<void> StoragePowerActor::appendCronEvent(
      const ChainEpoch &epoch, const CronEvent &event) {
    return Multimap::append(cron_event_queue, epoch, event);
  }
}  // namespace fc::vm::actor::builtin::storage_power
