/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::storage_power {

  struct PowerActorState : v2::storage_power::PowerActorState {
    outcome::result<Buffer> toCbor() const override;
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

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v3::storage_power::PowerActorState> {
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
}  // namespace fc
