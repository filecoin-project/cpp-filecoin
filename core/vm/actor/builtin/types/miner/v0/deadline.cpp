/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v0/deadline.hpp"

#include "common/error_text.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::RleBitset;

  outcome::result<PowerPair> Deadline::recordFaults(
      Runtime &runtime,
      const Sectors &sectors,
      SectorSize ssize,
      const QuantSpec &quant,
      ChainEpoch fault_expiration_epoch,
      const PartitionSectorMap &partition_sectors) {
    RleBitset partitions_with_faults;
    PowerPair new_faulty_power;

    for (const auto &[part_id, sector_nos] : partition_sectors.map) {
      OUTCOME_TRY(partition, this->partitions.get(part_id));
      OUTCOME_TRY(result,
                  partition->recordFaults(runtime,
                                          sectors,
                                          sector_nos,
                                          fault_expiration_epoch,
                                          ssize,
                                          quant));
      const auto &[new_faults,
                   partition_power_delta,
                   partition_new_faulty_power] = result;

      new_faulty_power += partition_new_faulty_power;
      if (!new_faults.empty()) {
        partitions_with_faults.insert(part_id);
      }

      OUTCOME_TRY(this->partitions.set(part_id, partition));
    }

    OUTCOME_TRY(addExpirationPartitions(
        fault_expiration_epoch, partitions_with_faults, quant));

    this->faulty_power += new_faulty_power;
    return new_faulty_power;
  }

  outcome::result<std::tuple<PowerPair, PowerPair>>
  Deadline::processDeadlineEnd(Runtime &runtime,
                               const QuantSpec &quant,
                               ChainEpoch fault_expiration_epoch) {
    PowerPair new_faulty_power;
    PowerPair failed_recovery_power;
    RleBitset rescheduled_partitions;

    OUTCOME_TRY(size, this->partitions.size());
    for (uint64_t part_id = 0; part_id < size; part_id++) {
      if (this->partitions_posted.has(part_id)) {
        continue;
      }

      OUTCOME_TRY(partition, this->partitions.get(part_id));
      if (partition->recovering_power.isZero()
          && partition->faulty_power == partition->live_power) {
        continue;
      }

      OUTCOME_TRY(result,
                  partition->recordMissedPostV0(
                      runtime, fault_expiration_epoch, quant));
      const auto &[part_faulty_power, part_failed_recovery_power] = result;

      if (!part_faulty_power.isZero()) {
        rescheduled_partitions.insert(part_id);
      }

      OUTCOME_TRY(this->partitions.set(part_id, partition));

      new_faulty_power += part_faulty_power;
      failed_recovery_power += part_failed_recovery_power;
    }

    OUTCOME_TRY(addExpirationPartitions(
        fault_expiration_epoch, rescheduled_partitions, quant));

    this->faulty_power += new_faulty_power;
    this->partitions_posted = {};

    return std::make_tuple(new_faulty_power, failed_recovery_power);
  }

  outcome::result<PoStResult> Deadline::recordProvenSectors(
      Runtime &runtime,
      const Sectors &sectors,
      SectorSize ssize,
      const QuantSpec &quant,
      ChainEpoch fault_expiration,
      const std::vector<PoStPartition> &post_partitions) {
    std::vector<RleBitset> all_sectors;
    std::vector<RleBitset> all_ignored;
    PowerPair new_faulty_power_total;
    PowerPair retracted_recovery_power_total;
    PowerPair recovered_power_total;
    RleBitset rescheduled_partitions;

    for (const auto &post : post_partitions) {
      if (this->partitions_posted.has(post.index)) {
        continue;
      }

      OUTCOME_TRY(partition, this->partitions.get(post.index));
      OUTCOME_TRY(
          result,
          partition->recordSkippedFaults(
              runtime, sectors, ssize, quant, fault_expiration, post.skipped));
      PowerPair new_fault_power;
      PowerPair retracted_recovery_power;
      std::tie(
          std::ignore, new_fault_power, retracted_recovery_power, std::ignore) =
          result;

      if (!new_fault_power.isZero()) {
        rescheduled_partitions.insert(post.index);
      }

      OUTCOME_TRY(recovered_power,
                  partition->recoverFaults(runtime, sectors, ssize, quant));
      OUTCOME_TRY(this->partitions.set(post.index, partition));

      new_faulty_power_total += new_fault_power;
      retracted_recovery_power_total += retracted_recovery_power;
      recovered_power_total += recovered_power;

      this->partitions_posted.insert(post.index);

      all_sectors.push_back(partition->sectors);
      all_ignored.push_back(partition->faults);
      all_ignored.push_back(partition->terminated);
    }

    OUTCOME_TRY(addExpirationPartitions(
        fault_expiration, rescheduled_partitions, quant));

    this->faulty_power =
        this->faulty_power - recovered_power_total + new_faulty_power_total;

    // Lotus gas conformance
    OUTCOME_TRY(this->partitions.amt.flush());

    RleBitset all_sector_nos;
    all_sector_nos += all_sectors;

    RleBitset all_ignored_sector_nos;
    all_ignored_sector_nos += all_ignored;

    return PoStResult{
        .power_delta = {},
        .new_faulty_power = new_faulty_power_total,
        .retracted_recovery_power = retracted_recovery_power_total,
        .recovered_power = recovered_power_total,
        .sectors = all_sector_nos,
        .ignored_sectors = all_ignored_sector_nos,
        .partitions = {}};
  }

  outcome::result<std::vector<SectorOnChainInfo>>
  Deadline::rescheduleSectorExpirations(
      Runtime &runtime,
      const Sectors &sectors,
      ChainEpoch expiration,
      const PartitionSectorMap &partition_sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    RleBitset rescheduled_partitions;

    for (const auto &[part_id, sector_nos] : partition_sectors.map) {
      OUTCOME_TRY(maybe_partition, this->partitions.tryGet(part_id));
      if (!maybe_partition) {
        // We failed to find the partition, it could have moved due to
        // compaction. This function is only reschedules sectors it can find so
        // we'll just skip it.
        continue;
      }

      auto &partition = maybe_partition.value();
      OUTCOME_TRY(moved,
                  partition->rescheduleExpirationsV0(
                      runtime, sectors, expiration, sector_nos, ssize, quant));
      if (moved.empty()) {
        // nothing moved
        continue;
      }

      rescheduled_partitions.insert(part_id);
      OUTCOME_TRY(this->partitions.set(part_id, partition));
    }

    if (!rescheduled_partitions.empty()) {
      OUTCOME_TRY(
          addExpirationPartitions(expiration, rescheduled_partitions, quant));
    }

    return std::vector<SectorOnChainInfo>{};
  }

  outcome::result<void> Deadline::validateState() const {
    // Do nothing for v0
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v0::miner
