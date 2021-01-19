/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
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
  using primitives::sector::Proof;
  using primitives::sector::RegisteredSealProof;

  /**
   * Type used in actor method parameters
   */
  struct SectorDeclaration {
    /**
     * The deadline to which the sectors are assigned, in range
     * [0..WPoStPeriodDeadlines)
     */
    uint64_t deadline{0};

    /** Partition index within the deadline containing the sectors. */
    uint64_t partition{0};

    /** Sectors in the partition being declared faulty. */
    RleBitset sectors;
  };
  CBOR_TUPLE(SectorDeclaration, deadline, partition, sectors)

  struct PowerPair {
    StoragePower raw;
    StoragePower qa;
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
    RegisteredSealProof registered_proof;
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
    RegisteredSealProof seal_proof;
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
    static outcome::result<MinerInfo> make(
        const Address &owner,
        const Address &worker,
        const std::vector<Address> &control,
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddrs,
        const RegisteredSealProof &seal_proof_type) {
      OUTCOME_TRY(sector_size,
                  primitives::sector::getSectorSize(seal_proof_type));
      OUTCOME_TRY(partition_sectors,
                  primitives::sector::getSealProofWindowPoStPartitionSectors(
                      seal_proof_type));
      return MinerInfo{.owner = owner,
                       .worker = worker,
                       .control = control,
                       .pending_worker_key = boost::none,
                       .peer_id = peer_id,
                       .multiaddrs = multiaddrs,
                       .seal_proof_type = seal_proof_type,
                       .sector_size = sector_size,
                       .window_post_partition_sectors = partition_sectors};
    }

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

    /**
     * Additional addresses that are permitted to submit messages controlling
     * this actor (optional). Must all be ID addresses.
     */
    std::vector<Address> control;

    boost::optional<WorkerKeyChange> pending_worker_key;

    /** Libp2p identity that should be used when connecting to this miner. */
    Buffer peer_id;

    /**
     * Slice of byte arrays representing Libp2p multi-addresses used for
     * establishing a connection with this miner.
     */
    std::vector<Multiaddress> multiaddrs;

    /** The proof type used by this miner for sealing sectors. */
    RegisteredSealProof seal_proof_type;

    /**
     * Amount of space in each sector committed to the network by this miner.
     * This is computed from the proof type and represented here redundantly.
     */
    SectorSize sector_size;

    /**
     * The number of sectors in each Window PoSt partition (proof). This is
     * computed from the proof type and represented here redundantly.
     */
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

  struct ExpirationSet {
    RleBitset on_time_sectors, early_sectors;
    TokenAmount on_time_pledge;
    PowerPair active_power;
    PowerPair faulty_power;
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
    PowerPair live_power;
    PowerPair faulty_power;
    PowerPair recovering_power;
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

  /** Deadline holds the state for all sectors due at a specific deadline */
  struct Deadline {
    /**
     * Makes empty deadline with adt::Array already flushed on ipld in order not
     * to charge extra gas for creation.
     * @param ipld - ipld with empty adt::Array stored
     * @param empty_amt_cid
     */
    inline static outcome::result<Deadline> makeEmpty(
        IpldPtr ipld, const CID &empty_amt_cid) {
      // construct with empty already cid stored in ipld to avoid gas charge
      Deadline deadline;
      deadline.partitions = adt::Array<Partition>(empty_amt_cid, ipld);
      deadline.expirations_epochs = adt::Array<RleBitset>(empty_amt_cid, ipld);
      ipld->load(deadline);
      return deadline;
    }

    /**
     * Partitions in this deadline, in order.
     * The keys of this AMT are always sequential integers beginning with zero.
     */
    adt::Array<Partition> partitions;

    /**
     * Maps epochs to partitions that _may_ have sectors that expire in or
     * before that epoch, either on-time or early as faults. Keys are quantized
     * to final epochs in each proving deadline.
     *
     * NOTE: Partitions MUST NOT be removed from this queue (until the
     * associated epoch has passed) even if they no longer have sectors expiring
     * at that epoch. Sectors expiring at this epoch may later be recovered, and
     * this queue will not be updated at that time.
     */
    adt::Array<RleBitset> expirations_epochs;

    /**
     * Partitions numbers with PoSt submissions since the proving period
     * started.
     */
    RleBitset post_submissions;

    /** Partitions with sectors that terminated early. */
    RleBitset early_terminations;

    /** The number of non-terminated sectors in this deadline (incl faulty). */
    uint64_t live_sectors{};

    /** The total number of sectors in this deadline (incl dead). */
    uint64_t total_sectors{};

    /** Memoized sum of faulty power in partitions. */
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

  /**
   * Deadlines contains Deadline objects, describing the sectors due at the
   * given deadline and their state (faulty, terminated, recovering, etc.).
   */
  struct Deadlines {
    /**
     * Makes empty deadlines with adt::Array already flushed on ipld in order
     * not to charge extra gas for creation.
     * @param ipld - ipld with empty adt::Array stored
     * @param empty_amt_cid
     */
    static outcome::result<Deadlines> makeEmpty(IpldPtr ipld,
                                                const CID &empty_amt_cid) {
      OUTCOME_TRY(deadline, Deadline::makeEmpty(ipld, empty_amt_cid));
      OUTCOME_TRY(deadline_cid, ipld->setCbor(deadline));
      return Deadlines{std::vector(kWPoStPeriodDeadlines, deadline_cid)};
    }

    std::vector<CID> due;
  };
  CBOR_TUPLE(Deadlines, due)

  enum class CronEventType {
    kWorkerKeyChange,
    kProvingDeadline,
    kProcessEarlyTerminations,
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

}  // namespace fc
