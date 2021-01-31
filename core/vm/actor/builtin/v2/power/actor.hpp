/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::power {
  using primitives::sector::RegisteredSealProof;
  using v0::storage_power::ChainEpoch;
  using v0::storage_power::ChainEpochKeyer;
  using v0::storage_power::CronEvent;
  using v0::storage_power::FilterEstimate;
  using v0::storage_power::SealVerifyInfo;
  using v0::storage_power::StoragePower;
  using v0::storage_power::TokenAmount;

  struct Claim {
    RegisteredSealProof seal_proof_type;
    StoragePower raw_power;
    StoragePower qa_power;
  };
  CBOR_TUPLE(Claim, seal_proof_type, raw_power, qa_power)

  struct State {
    StoragePower total_raw_power;
    StoragePower total_raw_commited;
    StoragePower total_qa_power;
    StoragePower total_qa_commited;
    TokenAmount total_pledge;
    StoragePower this_epoch_raw_power;
    StoragePower this_epoch_qa_power;
    TokenAmount this_epoch_pledge;
    FilterEstimate this_epoch_qa_power_smoothed;
    size_t miner_count;
    size_t num_miners_meeting_min_power;
    adt::Map<adt::Array<CronEvent>, ChainEpochKeyer> cron_event_queue;
    ChainEpoch first_cron_epoch;
    adt::Map<Claim, adt::AddressKeyer> claims;
    boost::optional<adt::Map<adt::Array<SealVerifyInfo>, adt::AddressKeyer>>
        proof_validation_batch;
  };
  CBOR_TUPLE(State,
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
             claims,
             proof_validation_batch)
}  // namespace fc::vm::actor::builtin::v2::power

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v2::power::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::power::State &state,
                     const Visitor &visit) {
      visit(state.cron_event_queue);
      visit(state.claims);
      if (state.proof_validation_batch) {
        visit(*state.proof_validation_batch);
      }
    }
  };
}  // namespace fc
