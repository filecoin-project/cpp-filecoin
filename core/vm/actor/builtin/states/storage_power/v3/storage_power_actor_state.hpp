/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/storage_power/v2/storage_power_actor_state.hpp"

#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::v3::storage_power {

  struct PowerActorState : v2::storage_power::PowerActorState {
   protected:
    outcome::result<void> check(bool condition) const override {
      return requireState(condition);
    }
  };

  CBOR_TUPLE(PowerActorState,
             total_raw_power,
             total_raw_commited,
             total_qa_power,
             total_qa_commited,
             total_pledge_collateral,
             this_epoch_raw_power,
             this_epoch_qa_power,
             this_epoch_pledge_collateral,
             this_epoch_qa_power_smoothed,
             miner_count,
             num_miners_meeting_min_power,
             cron_event_queue,
             first_cron_epoch,
             claims,
             proof_validation_batch)

}  // namespace fc::vm::actor::builtin::v3::storage_power

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v3::storage_power::PowerActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v3::storage_power::PowerActorState &state,
        const Visitor &visit) {
      visit(state.cron_event_queue);
      visit(state.claims);
      if (state.proof_validation_batch) {
        visit(*state.proof_validation_batch);
      }
    }
  };
}  // namespace fc::cbor_blake
