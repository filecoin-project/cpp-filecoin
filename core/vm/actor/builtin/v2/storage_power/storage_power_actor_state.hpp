/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>
#include "adt/address_key.hpp"
#include "adt/multimap.hpp"
#include "adt/uvarint_key.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::SealVerifyInfo;
  using runtime::Runtime;
  using v0::storage_power::Claim;
  using v0::storage_power::CronEvent;
  using ChainEpochKeyer = adt::VarintKeyer;

  struct State {
    static State empty(const IpldPtr &ipld);

    outcome::result<void> addToClaim(const Runtime &runtime,
                                     const Address &miner,
                                     const StoragePower &raw,
                                     const StoragePower &qa);
    outcome::result<void> deleteClaim(const Runtime &runtime,
                                      const Address &miner);
    outcome::result<void> addPledgeTotal(const Runtime &runtime,
                                         const TokenAmount &amount);
    outcome::result<void> appendCronEvent(const ChainEpoch &epoch,
                                          const CronEvent &event);
    void updateSmoothedEstimate(int64_t delta);
    std::tuple<StoragePower, StoragePower> getCurrentTotalPower() const;

    StoragePower total_raw_power;

    /** includes claims from miners below min power threshold */
    StoragePower total_raw_commited;
    StoragePower total_qa_power;

    /** includes claims from miners below min power threshold */
    StoragePower total_qa_commited;
    TokenAmount total_pledge;

    /**
     * These fields are set once per epoch in the previous cron tick and used
     * for consistent values across a single epoch's state transition.
     */
    StoragePower this_epoch_raw_power;
    StoragePower this_epoch_qa_power;
    TokenAmount this_epoch_pledge;
    FilterEstimate this_epoch_qa_power_smoothed;

    size_t miner_count;
    size_t num_miners_meeting_min_power;
    adt::Map<adt::Array<CronEvent>, ChainEpochKeyer> cron_event_queue;

    /**
     * First epoch in which a cron task may be stored.
     * Cron will iterate every epoch between this and the current epoch
     * inclusively to find tasks to execute.
     */
    ChainEpoch first_cron_epoch;
    adt::Map<Claim, adt::AddressKeyer> claims;
    boost::optional<adt::Map<adt::Array<SealVerifyInfo>, adt::AddressKeyer>>
        proof_validation_batch;
  };

  using StoragePowerActorState = State;

  CBOR_TUPLE(StoragePowerActorState,
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

}  // namespace fc::vm::actor::builtin::v2::storage_power

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v2::storage_power::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::storage_power::State &state,
                     const Visitor &visit) {
      visit(state.cron_event_queue);
      visit(state.claims);
      if (state.proof_validation_batch) {
        visit(*state.proof_validation_batch);
      }
    }
  };
}  // namespace fc
