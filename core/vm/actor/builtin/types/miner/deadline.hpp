/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "vm/actor/builtin/types/miner/dispute_info.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/partition.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/miner/post_partition.hpp"
#include "vm/actor/builtin/types/miner/post_result.hpp"
#include "vm/actor/builtin/types/miner/sectors.hpp"
#include "vm/actor/builtin/types/miner/termination.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;

  constexpr size_t kPartitionsBitWidth = 3;
  constexpr size_t kExpirationsBitWidth = 5;
  constexpr size_t kPoStSubmissionBitWidth = 3;

  /** Deadline holds the state for all sectors due at a specific deadline */
  struct Deadline {
    virtual ~Deadline() = default;

    /**
     * Partitions in this deadline, in order.
     * The keys of this AMT are always sequential integers beginning with zero.
     */
    adt::Array<Universal<Partition>, kPartitionsBitWidth> partitions;

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
    adt::Array<RleBitset, kExpirationsBitWidth> expirations_epochs;

    /**
     * Partitions that have been proved by window PoSts so far during the
     * current challenge window.
     */
    RleBitset partitions_posted;  // old post_submissions

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
    adt::Array<WindowedPoSt, kPoStSubmissionBitWidth>
        optimistic_post_submissions;

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

    // Methods

    outcome::result<void> addExpirationPartitions(
        ChainEpoch expiration_epoch,
        const RleBitset &partition_set,
        const QuantSpec &quant);

    outcome::result<ExpirationSet> popExpiredSectors(Runtime &runtime,
                                                     ChainEpoch until,
                                                     const QuantSpec &quant);

    outcome::result<PowerPair> addSectors(
        Runtime &runtime,
        uint64_t partition_size,
        bool proven,
        std::vector<SectorOnChainInfo> sectors,
        SectorSize ssize,
        const QuantSpec &quant);

    outcome::result<std::tuple<TerminationResult, bool>> popEarlyTerminations(
        Runtime &runtime, uint64_t max_partitions, uint64_t max_sectors);

    outcome::result<std::tuple<RleBitset, bool>> popExpiredPartitions(
        ChainEpoch until, const QuantSpec &quant);

    outcome::result<PowerPair> terminateSectors(
        Runtime &runtime,
        const Sectors &sectors,
        ChainEpoch epoch,
        const PartitionSectorMap &partition_sectors,
        SectorSize ssize,
        const QuantSpec &quant);

    outcome::result<std::tuple<RleBitset, RleBitset, PowerPair>>
    removePartitions(Runtime &runtime,
                     const RleBitset &to_remove,
                     const QuantSpec &quant);

    virtual outcome::result<PowerPair> recordFaults(
        Runtime &runtime,
        const Sectors &sectors,
        SectorSize ssize,
        const QuantSpec &quant,
        ChainEpoch fault_expiration_epoch,
        const PartitionSectorMap &partition_sectors) = 0;

    outcome::result<void> declareFaultsRecovered(
        const Sectors &sectors,
        SectorSize ssize,
        const PartitionSectorMap &partition_sectors);

    virtual outcome::result<std::tuple<PowerPair, PowerPair>>
    processDeadlineEnd(Runtime &runtime,
                       const QuantSpec &quant,
                       ChainEpoch fault_expiration_epoch) = 0;

    virtual outcome::result<PoStResult> recordProvenSectors(
        Runtime &runtime,
        const Sectors &sectors,
        SectorSize ssize,
        const QuantSpec &quant,
        ChainEpoch fault_expiration,
        const std::vector<PoStPartition> &post_partitions) = 0;

    virtual outcome::result<std::vector<SectorOnChainInfo>>
    rescheduleSectorExpirations(Runtime &runtime,
                                const Sectors &sectors,
                                ChainEpoch expiration,
                                const PartitionSectorMap &partition_sectors,
                                SectorSize ssize,
                                const QuantSpec &quant) = 0;

    virtual outcome::result<void> validateState() const = 0;

    outcome::result<DisputeInfo> loadPartitionsForDispute(
        const RleBitset &partition_set) const;
  };

}  // namespace fc::vm::actor::builtin::types::miner
