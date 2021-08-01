/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "vm/actor/builtin/types/miner/partition.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;

  /** Deadline holds the state for all sectors due at a specific deadline */
  struct Deadline {
    Deadline() = default;
    Deadline(const Deadline &other) = default;
    Deadline(Deadline &&other) = default;

    /**
     * Makes empty deadline with adt::Array already flushed on ipld in order not
     * to charge extra gas for creation.
     * @param ipld - ipld with empty adt::Array stored
     * @param empty_amt_cid
     */
    inline static Deadline makeEmpty(IpldPtr ipld, const CID &empty_amt_cid) {
      // TODO (a.chernyshov) initialize amt with correct bitwidth
      // construct with empty already cid stored in ipld to avoid gas charge
      Deadline deadline;
      deadline.partitions = {empty_amt_cid, ipld};
      deadline.expirations_epochs = {empty_amt_cid, ipld};
      return deadline;
    }

    /**
     * Partitions in this deadline, in order.
     * The keys of this AMT are always sequential integers beginning with zero.
     */
    adt::Array<Universal<Partition>, 3> partitions;

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
    adt::Array<RleBitset, 5> expirations_epochs;

    /**
     * Partitions numbers with PoSt submissions since the proving period
     * started.
     */
    RleBitset post_submissions;

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
    adt::Array<WindowedPoSt, 2> optimistic_post_submissions;

    /**
     * Snapshot of partition state at the end of the previous challenge window
     * for this deadline.
     */
    decltype(partitions) partitions_snapshot;

    /**
     * These proofs may be disputed via DisputeWindowedPoSt. Successfully
     * disputed window PoSts are removed from the snapshot.
     */
    decltype(optimistic_post_submissions) optimistic_post_submissions_snapshot;
  };

  /**
   * Deadlines contains Deadline objects, describing the sectors due at the
   * given deadline and their state (faulty, terminated, recovering, etc.).
   */
  struct Deadlines {
    std::vector<CID> due;  // Deadline
  };
  CBOR_TUPLE(Deadlines, due)

}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::types::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::Deadline &p,
                     const Visitor &visit) {
      visit(p.partitions);
      visit(p.expirations_epochs);
      visit(p.optimistic_post_submissions);
    }
  };
}  // namespace fc::cbor_blake
