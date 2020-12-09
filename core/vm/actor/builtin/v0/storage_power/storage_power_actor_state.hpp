/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_STORAGE_POWER_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_STORAGE_POWER_ACTOR_STATE_HPP

#include "adt/address_key.hpp"
#include "adt/multimap.hpp"
#include "adt/uvarint_key.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::FilterEstimate;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::SealVerifyInfo;
  using ChainEpochKeyer = adt::VarintKeyer;

  struct Claim {
    StoragePower raw_power, qa_power;

    inline bool operator==(const Claim &other) const {
      return raw_power == other.raw_power && qa_power == other.qa_power;
    }
  };
  CBOR_TUPLE(Claim, raw_power, qa_power)

  struct CronEvent {
    Address miner_address;
    Buffer callback_payload;
  };
  CBOR_TUPLE(CronEvent, miner_address, callback_payload)

  struct State {
    static State empty(IpldPtr ipld);
    outcome::result<void> addToClaim(const Address &miner,
                                     const StoragePower &raw,
                                     const StoragePower &qa);
    outcome::result<void> addPledgeTotal(const TokenAmount &amount);
    outcome::result<void> appendCronEvent(const ChainEpoch &epoch,
                                          const CronEvent &event);

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
    ChainEpoch last_epoch_tick;
    adt::Map<Claim, adt::AddressKeyer> claims;
    boost::optional<adt::Map<adt::Array<SealVerifyInfo>, adt::AddressKeyer>>
        proof_validation_batch;
  };

  using StoragePowerActorState = State;
  using StoragePowerActor = StoragePowerActorState;

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
             last_epoch_tick,
             claims,
             proof_validation_batch)

}  // namespace fc::vm::actor::builtin::v0::storage_power

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::storage_power::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::storage_power::State &state,
                     const Visitor &visit) {
      visit(state.cron_event_queue);
      visit(state.claims);
      if (state.proof_validation_batch) {
        visit(*state.proof_validation_batch);
      }
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_STORAGE_POWER_ACTOR_STATE_HPP
