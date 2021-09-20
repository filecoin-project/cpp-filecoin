/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v3/deadline.hpp"

#include "common/error_text.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::RleBitset;

  outcome::result<std::tuple<PowerPair, PowerPair>>
  Deadline::processDeadlineEnd(Runtime &runtime,
                               const QuantSpec &quant,
                               ChainEpoch fault_expiration_epoch) {
    OUTCOME_TRY(result,
                v2::miner::Deadline::processDeadlineEnd(
                    runtime, quant, fault_expiration_epoch));

    this->partitions_snapshot = this->partitions;
    this->optimistic_post_submissions_snapshot =
        this->optimistic_post_submissions;
    this->optimistic_post_submissions = {};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(),
                        this->optimistic_post_submissions);

    return std::move(result);
  }

  outcome::result<PoStResult> Deadline::recordProvenSectors(
      Runtime &runtime,
      const Sectors &sectors,
      SectorSize ssize,
      const QuantSpec &quant,
      ChainEpoch fault_expiration,
      const std::vector<PoStPartition> &post_partitions) {
    RleBitset partition_indexes;
    for (const auto &partition : post_partitions) {
      partition_indexes.insert(partition.index);
    }

    if (partition_indexes.size() != post_partitions.size()) {
      return ERROR_TEXT("duplicate partitions proven");
    }

    auto already_proven = this->partitions_posted.intersect(partition_indexes);
    if (!already_proven.empty()) {
      return ERROR_TEXT("partition already proven");
    }

    std::vector<RleBitset> all_sectors;
    std::vector<RleBitset> all_ignored;
    PowerPair new_faulty_power_total;
    PowerPair retracted_recovery_power_total;
    PowerPair recovered_power_total;
    PowerPair power_delta;
    RleBitset rescheduled_partitions;

    for (const auto &post : post_partitions) {
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
        .partitions = partition_indexes};
  }

}  // namespace fc::vm::actor::builtin::v3::miner
