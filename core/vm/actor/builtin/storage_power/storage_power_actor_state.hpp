/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP

#include "adt/address_key.hpp"
#include "adt/multimap.hpp"
#include "adt/uvarint_key.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::storage_power {
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
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

    StoragePower total_raw_power, total_qa_power;
    TokenAmount total_pledge;
    size_t miner_count;
    adt::Map<adt::Array<CronEvent>, ChainEpochKeyer> cron_event_queue;
    ChainEpoch last_epoch_tick;
    adt::Map<Claim, adt::AddressKeyer> claims;
    size_t num_miners_meeting_min_power;
  };

  using StoragePowerActorState = State;
  using StoragePowerActor = StoragePowerActorState;

  CBOR_TUPLE(StoragePowerActorState,
             total_raw_power,
             total_qa_power,
             total_pledge,
             miner_count,
             cron_event_queue,
             last_epoch_tick,
             claims,
             num_miners_meeting_min_power)

}  // namespace fc::vm::actor::builtin::storage_power

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::storage_power::State> {
    template <typename F>
    static void f(vm::actor::builtin::storage_power::State &s, const F &f) {
      f(s.cron_event_queue);
      f(s.claims);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
