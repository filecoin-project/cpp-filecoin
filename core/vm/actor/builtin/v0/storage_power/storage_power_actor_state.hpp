/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/builtin/states/power_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using primitives::StoragePower;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;
  using types::storage_power::Claim;

  struct PowerActorState : states::PowerActorState {
    outcome::result<Buffer> toCbor() const override;

    std::shared_ptr<states::PowerActorState> copy() const override;

    outcome::result<void> deleteClaim(const Runtime &runtime,
                                      const Address &address) override;

   protected:
    std::tuple<bool, bool> claimsAreBelow(
        const Claim &old_claim, const Claim &new_claim) const override;
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
             last_processed_cron_epoch,
             claims,
             proof_validation_batch)

}  // namespace fc::vm::actor::builtin::v0::storage_power

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::storage_power::PowerActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v0::storage_power::PowerActorState &state,
        const Visitor &visit) {
      visit(state.cron_event_queue);
      visit(state.claims);
      if (state.proof_validation_batch) {
        visit(*state.proof_validation_batch);
      }
    }
  };
}  // namespace fc
