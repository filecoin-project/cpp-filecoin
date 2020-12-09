/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V0_MINER_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V0_MINER_TYPES_HPP

#include "adt/array.hpp"
#include "adt/map.hpp"
#include "adt/uvarint_key.hpp"
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/v0/miner/policy.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using adt::UvarintKeyer;
  using common::Buffer;
  using crypto::randomness::Randomness;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::OnChainPoStVerifyInfo;
  using primitives::sector::Proof;

  struct PowerPair {
    StoragePower raw, qa;
  };
  CBOR_TUPLE(PowerPair, raw, qa)

  struct VestingFunds {
    struct Fund {
      ChainEpoch epoch;
      TokenAmount amount;
    };
    std::vector<Fund> funds;
  };
  CBOR_TUPLE(VestingFunds::Fund, epoch, amount)
  CBOR_TUPLE(VestingFunds, funds)

  struct SectorPreCommitInfo {
    RegisteredProof registered_proof;
    SectorNumber sector;
    /// CommR
    CID sealed_cid;
    ChainEpoch seal_epoch;
    std::vector<DealId> deal_ids;
    /// Sector expiration
    ChainEpoch expiration;
    bool replace_capacity = false;
    uint64_t replace_deadline, replace_partition;
    SectorNumber replace_sector;
  };
  CBOR_TUPLE(SectorPreCommitInfo,
             registered_proof,
             sector,
             sealed_cid,
             seal_epoch,
             deal_ids,
             expiration,
             replace_capacity,
             replace_deadline,
             replace_partition,
             replace_sector)

  struct SectorPreCommitOnChainInfo {
    SectorPreCommitInfo info;
    TokenAmount precommit_deposit;
    ChainEpoch precommit_epoch;
    DealWeight deal_weight;
    DealWeight verified_deal_weight;
  };
  CBOR_TUPLE(SectorPreCommitOnChainInfo,
             info,
             precommit_deposit,
             precommit_epoch,
             deal_weight,
             verified_deal_weight)

  struct SectorOnChainInfo {
    SectorNumber sector;
    RegisteredProof seal_proof;
    CID sealed_cid;
    std::vector<DealId> deals;
    ChainEpoch activation_epoch;
    ChainEpoch expiration;
    DealWeight deal_weight;
    DealWeight verified_deal_weight;
    TokenAmount init_pledge, expected_day_reward, expected_storage_pledge;
  };
  CBOR_TUPLE(SectorOnChainInfo,
             sector,
             seal_proof,
             sealed_cid,
             deals,
             activation_epoch,
             expiration,
             deal_weight,
             verified_deal_weight,
             init_pledge,
             expected_day_reward,
             expected_storage_pledge)

  struct WorkerKeyChange {
    /// Must be an ID address
    Address new_worker;
    ChainEpoch effective_at;
  };
  CBOR_TUPLE(WorkerKeyChange, new_worker, effective_at)

  struct MinerInfo {
    /**
     * Account that owns this miner.
     * - Income and returned collateral are paid to this address.
     * - This address is also allowed to change the worker address for the
     * miner.
     *
     * Must be an ID-address.
     */
    Address owner;
    /**
     * Worker account for this miner. The associated pubkey-type address is used
     * to sign blocks and messages on behalf of this miner. Must be an
     * ID-address.
     */
    Address worker;
    std::vector<Address> control;
    boost::optional<WorkerKeyChange> pending_worker_key;
    /// Libp2p identity that should be used when connecting to this miner.
    Buffer peer_id;
    std::vector<Multiaddress> multiaddrs;
    RegisteredProof seal_proof_type;
    /// Amount of space in each sector committed to the network by this miner.
    SectorSize sector_size;
    uint64_t window_post_partition_sectors;
  };
  CBOR_TUPLE(MinerInfo,
             owner,
             worker,
             control,
             pending_worker_key,
             peer_id,
             multiaddrs,
             seal_proof_type,
             sector_size,
             window_post_partition_sectors)

  struct DeadlineInfo {
    ChainEpoch current_epoch;
    ChainEpoch period_start;
    uint64_t index;
    ChainEpoch open;
    ChainEpoch close;
    ChainEpoch challenge;
    ChainEpoch fault_cutoff;

    static DeadlineInfo make(ChainEpoch start, size_t id, ChainEpoch now);
    auto nextNotElapsed() const {
      return elapsed() ? make(nextPeriodStart(), index, current_epoch) : *this;
    }
    ChainEpoch nextPeriodStart() const {
      return period_start + kWPoStProvingPeriod;
    }
    bool elapsed() const {
      return current_epoch >= close;
    }
    bool faultCutoffPassed() const {
      return current_epoch >= fault_cutoff;
    }
    bool periodStarted() const {
      return current_epoch >= period_start;
    }
    ChainEpoch periodEnd() const {
      return period_start + kWPoStProvingPeriod - 1;
    }
    auto last() const {
      return close - 1;
    }
  };

  struct ExpirationSet {
    RleBitset on_time_sectors, early_sectors;
    TokenAmount on_time_pledge;
    PowerPair active_power, faulty_power;
  };
  CBOR_TUPLE(ExpirationSet,
             on_time_sectors,
             early_sectors,
             on_time_pledge,
             active_power,
             faulty_power)

  struct Partition {
    RleBitset sectors, faults, recoveries, terminated;
    adt::Array<ExpirationSet> expirations_epochs;  // quanted
    adt::Array<RleBitset> early_terminated;
    PowerPair live_power, faulty_power, recovering_power;
  };
  CBOR_TUPLE(Partition,
             sectors,
             faults,
             recoveries,
             terminated,
             expirations_epochs,
             early_terminated,
             live_power,
             faulty_power,
             recovering_power)

  struct Deadline {
    adt::Array<Partition> partitions;
    adt::Array<RleBitset> expirations_epochs;
    RleBitset post_submissions, early_terminations;
    uint64_t live_sectors{}, total_sectors{};
    PowerPair faulty_power;
  };
  CBOR_TUPLE(Deadline,
             partitions,
             expirations_epochs,
             post_submissions,
             early_terminations,
             live_sectors,
             total_sectors,
             faulty_power)

  struct Deadlines {
    std::vector<CIDT<Deadline>> due;
  };
  CBOR_TUPLE(Deadlines, due)

  /// Balance of a Actor should equal exactly the sum of PreCommit deposits
  struct State {
    CIDT<MinerInfo> info;
    TokenAmount precommit_deposit;
    TokenAmount locked_funds;
    CIDT<VestingFunds> vesting_funds;
    TokenAmount initial_pledge;
    adt::Map<SectorPreCommitOnChainInfo, UvarintKeyer> precommitted_sectors;
    adt::Array<RleBitset> precommitted_expiry;
    CIDT<RleBitset> allocated_sectors;
    adt::Array<SectorOnChainInfo> sectors;
    ChainEpoch proving_period_start{};
    uint64_t current_deadline{};
    CIDT<Deadlines> deadlines;
    RleBitset early_terminations;

    DeadlineInfo deadlineInfo(ChainEpoch now) const;
    outcome::result<void> addFaults(const RleBitset &sectors, ChainEpoch epoch);
    template <typename C>
    outcome::result<std::vector<SectorOnChainInfo>> getSectors(const C &c) {
      std::vector<SectorOnChainInfo> result;
      for (auto i : c) {
        OUTCOME_TRY(sector, sectors.get(i));
        result.push_back(std::move(sector));
      }
      return std::move(result);
    }
  };
  CBOR_TUPLE(State,
             info,
             precommit_deposit,
             locked_funds,
             vesting_funds,
             initial_pledge,
             precommitted_sectors,
             precommitted_expiry,
             allocated_sectors,
             sectors,
             proving_period_start,
             current_deadline,
             deadlines,
             early_terminations)
  using MinerActorState = State;

  enum class CronEventType {
    WorkerKeyChange,
    ProvingDeadline,
    ProcessEarlyTerminations,
  };

  struct CronEventPayload {
    CronEventType event_type;
  };
  CBOR_TUPLE(CronEventPayload, event_type)
}  // namespace fc::vm::actor::builtin::v0::miner

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::miner::Partition> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::Partition &p,
                     const Visitor &visit) {
      visit(p.expirations_epochs);
      visit(p.early_terminated);
    }
  };
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::Deadline &d,
                     const Visitor &visit) {
      visit(d.partitions);
      visit(d.expirations_epochs);
    }
  };
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::miner::Deadlines> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::Deadlines &ds,
                     const Visitor &visit) {
      for (auto &d : ds.due) {
        visit(d);
      }
    }
  };
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::miner::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::State &state,
                     const Visitor &visit) {
      visit(state.info);
      visit(state.vesting_funds);
      visit(state.precommitted_sectors);
      visit(state.precommitted_expiry);
      visit(state.allocated_sectors);
      visit(state.sectors);
      visit(state.deadlines);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V0_MINER_TYPES_HPP
