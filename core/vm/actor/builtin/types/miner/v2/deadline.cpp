/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v2/deadline.hpp"

#include "common/error_text.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::RleBitset;

  outcome::result<PowerPair> Deadline::recordFaults(
      Runtime &runtime,
      const Sectors &sectors,
      SectorSize ssize,
      const QuantSpec &quant,
      ChainEpoch fault_expiration_epoch,
      const PartitionSectorMap &partition_sectors) {
    RleBitset partitions_with_faults;
    PowerPair power_delta;

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

      this->faulty_power += partition_new_faulty_power;
      power_delta += partition_power_delta;
      if (!new_faults.empty()) {
        partitions_with_faults.insert(part_id);
      }

      OUTCOME_TRY(this->partitions.set(part_id, partition));
    }

    OUTCOME_TRY(addExpirationPartitions(
        fault_expiration_epoch, partitions_with_faults, quant));

    return power_delta;
  }

  outcome::result<std::tuple<PowerPair, PowerPair>>
  Deadline::processDeadlineEnd(Runtime &runtime,
                               const QuantSpec &quant,
                               ChainEpoch fault_expiration_epoch) {
    PowerPair power_delta;
    PowerPair penalized_power;
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
                  partition->recordMissedPostV2(
                      runtime, fault_expiration_epoch, quant));
      const auto &[part_power_delta,
                   part_penalized_power,
                   part_new_faulty_power] = result;

      if (!part_new_faulty_power.isZero()) {
        rescheduled_partitions.insert(part_id);
      }

      OUTCOME_TRY(this->partitions.set(part_id, partition));

      this->faulty_power += part_new_faulty_power;
      power_delta += part_power_delta;
      penalized_power += part_penalized_power;
    }

    OUTCOME_TRY(addExpirationPartitions(
        fault_expiration_epoch, rescheduled_partitions, quant));

    this->partitions_posted = {};

    return std::make_tuple(power_delta, penalized_power);
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
    PowerPair power_delta;
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
      auto &[new_power_delta,
             new_fault_power,
             retracted_recovery_power,
             has_new_faults] = result;

      if (has_new_faults) {
        rescheduled_partitions.insert(post.index);
      }

      OUTCOME_TRY(recovered_power,
                  partition->recoverFaults(runtime, sectors, ssize, quant));

      new_power_delta += partition->activateUnproven();

      OUTCOME_TRY(this->partitions.set(post.index, partition));

      new_faulty_power_total += new_fault_power;
      retracted_recovery_power_total += retracted_recovery_power;
      recovered_power_total += recovered_power;
      power_delta += new_power_delta + recovered_power;

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
        .power_delta = power_delta,
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
    std::vector<SectorOnChainInfo> all_replaced;

    for (const auto &[part_id, sector_nos] : partition_sectors.map) {
      OUTCOME_TRY(maybe_partition, this->partitions.tryGet(part_id));
      if (!maybe_partition) {
        // We failed to find the partition, it could have moved due to
        // compaction. This function is only reschedules sectors it can find so
        // we'll just skip it.
        continue;
      }

      auto &partition = maybe_partition.value();
      OUTCOME_TRY(replaced,
                  partition->rescheduleExpirationsV2(
                      runtime, sectors, expiration, sector_nos, ssize, quant));
      if (replaced.empty()) {
        // nothing moved
        continue;
      }

      all_replaced.insert(all_replaced.end(), replaced.begin(), replaced.end());

      rescheduled_partitions.insert(part_id);
      OUTCOME_TRY(this->partitions.set(part_id, partition));
    }

    if (!rescheduled_partitions.empty()) {
      OUTCOME_TRY(
          addExpirationPartitions(expiration, rescheduled_partitions, quant));
    }

    return all_replaced;
  }

  outcome::result<void> Deadline::validateState() const {
    if (this->live_sectors > this->total_sectors) {
      return ERROR_TEXT("Deadline left with more live sectors than total");
    }

    if (this->faulty_power.raw < 0 || this->faulty_power.qa < 0) {
      return ERROR_TEXT("Deadline left with negative faulty power");
    }

    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v2::miner
