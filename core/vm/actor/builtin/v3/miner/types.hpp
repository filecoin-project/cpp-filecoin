/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/v0/miner/types.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using common::Buffer;
  using primitives::RleBitset;
  using primitives::SectorSize;
  using primitives::address::Address;
  using primitives::sector::PoStProof;
  using primitives::sector::RegisteredPoStProof;
  using v0::miner::kWPoStPeriodDeadlines;
  using v0::miner::Partition;
  using v0::miner::PowerPair;
  using v0::miner::WorkerKeyChange;

  struct MinerInfo {
    static outcome::result<MinerInfo> make(
        const Address &owner,
        const Address &worker,
        const std::vector<Address> &control,
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddrs,
        const RegisteredPoStProof &window_post_proof_type) {
      OUTCOME_TRY(sector_size,
                  primitives::sector::getSectorSize(window_post_proof_type));
      OUTCOME_TRY(partition_sectors,
                  primitives::sector::getWindowPoStPartitionSectors(
                      window_post_proof_type));
      return MinerInfo{.owner = owner,
                       .worker = worker,
                       .control = control,
                       .pending_worker_key = boost::none,
                       .peer_id = peer_id,
                       .multiaddrs = multiaddrs,
                       .window_post_proof_type = window_post_proof_type,
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

    /**
     * The proof type used for Window PoSt for this miner.
     * A miner may commit sectors with different seal proof types (but
     * compatible sector size and corresponding PoSt proof types).
     */
    RegisteredPoStProof window_post_proof_type;

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
             window_post_proof_type,
             sector_size,
             window_post_partition_sectors)

  struct WindowedPoSt {
    /**
     * Partitions proved by this WindowedPoSt.
     */
    RleBitset partitions;

    /**
     * Array of proofs, one per distinct registered proof type present in the
     * sectors being proven. In the usual case of a single proof type, this
     * array will always have a single element (independent of number of
     * partitions).
     */
    std::vector<PoStProof> proofs;
  };
  CBOR_TUPLE(WindowedPoSt, partitions, proofs)

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
      // TODO (a.chernyshov) initialize amt with correct bitwidth
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
     * Partitions that have been proved by window PoSts so far during the
     * current challenge window.
     */
    RleBitset partitions_posted;

    /** Partitions with sectors that terminated early. */
    RleBitset early_terminations;

    /** The number of non-terminated sectors in this deadline (incl faulty). */
    uint64_t live_sectors{};

    /** The total number of sectors in this deadline (incl dead). */
    uint64_t total_sectors{};

    /** Memoized sum of faulty power in partitions. */
    PowerPair faulty_power;

    /**
     * AMT of optimistically accepted WindowPoSt proofs, submitted during the
     * current challenge window. At the end of the challenge window, this AMT
     * will be moved to PoStSubmissionsSnapshot. WindowPoSt proofs verified
     * on-chain do not appear in this AMT.
     */
    adt::Array<WindowedPoSt> optimistic_post_submissions;

    /**
     * Snapshot of partition state at the end of the previous challenge window
     * for this deadline.
     */
    CID partitions_snapshot;

    /**
     * These proofs may be disputed via DisputeWindowedPoSt. Successfully
     * disputed window PoSts are removed from the snapshot.
     */
    CID optimistic_post_submissions_snapshot;
  };
  CBOR_TUPLE(Deadline,
             partitions,
             expirations_epochs,
             partitions_posted,
             early_terminations,
             live_sectors,
             total_sectors,
             faulty_power,
             optimistic_post_submissions,
             partitions_snapshot,
             optimistic_post_submissions_snapshot)

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
      return Deadlines{
          std::vector(kWPoStPeriodDeadlines, CIDT<Deadline>(deadline_cid))};
    }

    std::vector<CIDT<Deadline>> due;
  };
  CBOR_TUPLE(Deadlines, due)

}  // namespace fc::vm::actor::builtin::v3::miner

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v3::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::miner::Deadline &d,
                     const Visitor &visit) {
      visit(d.partitions);
      visit(d.expirations_epochs);
      visit(d.optimistic_post_submissions);
    }
  };

  template <>
  struct Ipld::Visit<vm::actor::builtin::v3::miner::Deadlines> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::miner::Deadlines &ds,
                     const Visitor &visit) {
      for (auto &d : ds.due) {
        visit(d);
      }
    }
  };

}  // namespace fc
