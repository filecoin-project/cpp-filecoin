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
  using types::storage_power::Claim;

  struct PowerActorState : states::PowerActorState {
    outcome::result<Buffer> toCbor() const override;

    std::shared_ptr<states::PowerActorState> copy() const override;

    outcome::result<void> setClaim(const fc::vm::runtime::Runtime &runtime,
                                   const Address &address,
                                   const StoragePower &raw,
                                   const StoragePower &qa,
                                   RegisteredSealProof seal_proof) override;

    outcome::result<void> deleteClaim(const fc::vm::runtime::Runtime &runtime,
                                      const Address &address) override;

    outcome::result<bool> hasClaim(const Address &address) const override;

    outcome::result<boost::optional<Claim>> tryGetClaim(
        const Address &address) const override;

    outcome::result<Claim> getClaim(const Address &address) const override;

    outcome::result<std::vector<adt::AddressKeyer::Key>> getClaimsKeys()
        const override;

    outcome::result<void> loadClaimsRoot() override;

   protected:
    std::tuple<bool, bool> claimsAreBelow(
        const Claim &old_claim, const Claim &new_claim) const override;
  };

  CBOR_TUPLE(PowerActorState,
             total_raw_power,
             total_raw_commited,
             total_qa_power,
             total_qa_commited,
             total_pledge,
             this_epoch_raw_power,
             this_epoch_qa_power,
             this_epoch_pledge,
             this_epoch_qa_power_smoothed,
             miner_count,
             num_miners_meeting_min_power,
             cron_event_queue,
             first_cron_epoch,
             last_processed_cron_epoch,
             claims0,
             proof_validation_batch)

}  // namespace fc::vm::actor::builtin::v0::storage_power

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v0::storage_power::PowerActorState> {
    template <typename Visitor>
    static void call(
        vm::actor::builtin::v0::storage_power::PowerActorState &state,
        const Visitor &visit) {
      visit(state.cron_event_queue);
      visit(state.claims0);
      if (state.proof_validation_batch) {
        visit(*state.proof_validation_batch);
      }
    }
  };
}  // namespace fc::cbor_blake
