/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline.hpp"

#include "common/container_utils.hpp"
#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/partition.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using common::slice;

  outcome::result<void> Deadline::addExpirationPartitions(
      ChainEpoch expiration_epoch,
      const RleBitset &partition_set,
      const QuantSpec &quant) {
    if (partition_set.empty()) {
      return outcome::success();
    }

    BitfieldQueue<kExpirationsBitWidth> queue{.queue = this->expirations_epochs,
                                              .quant = quant};

    OUTCOME_TRY(queue.addToQueue(expiration_epoch, partition_set));
    this->expirations_epochs = queue.queue;

    return outcome::success();
  }

  outcome::result<ExpirationSet> Deadline::popExpiredSectors(
      Runtime &runtime, ChainEpoch until, const QuantSpec &quant) {
    OUTCOME_TRY(result, popExpiredPartitions(until, quant));
    const auto &[expired_partitions, modified] = result;
    if (!modified) {
      return ExpirationSet{};
    }

    std::vector<RleBitset> on_time_sectors;
    std::vector<RleBitset> early_sectors;
    TokenAmount all_on_time_pledge{0};
    PowerPair all_active_power;
    PowerPair all_faulty_power;
    std::vector<uint64_t> partitions_with_early_terminations;

    for (const auto &part_id : expired_partitions) {
      OUTCOME_TRY(partition, this->partitions.get(part_id));
      OUTCOME_TRY(part_expiration,
                  partition->popExpiredSectors(runtime, until, quant));

      on_time_sectors.push_back(part_expiration.on_time_sectors);
      early_sectors.push_back(part_expiration.early_sectors);
      all_active_power += part_expiration.active_power;
      all_faulty_power += part_expiration.faulty_power;
      all_on_time_pledge += part_expiration.on_time_pledge;

      if (!part_expiration.early_sectors.empty()) {
        partitions_with_early_terminations.push_back(part_id);
      }

      OUTCOME_TRY(this->partitions.set(part_id, partition));
    }

    for (const auto &part_id : partitions_with_early_terminations) {
      this->early_terminations.insert(part_id);
    }

    RleBitset all_on_time_sectors;
    all_on_time_sectors += on_time_sectors;

    RleBitset all_early_sectors;
    all_early_sectors += early_sectors;

    this->live_sectors -= all_on_time_sectors.size() + all_early_sectors.size();
    this->faulty_power -= all_faulty_power;

    return ExpirationSet{.on_time_sectors = all_on_time_sectors,
                         .early_sectors = all_early_sectors,
                         .on_time_pledge = all_on_time_pledge,
                         .active_power = all_active_power,
                         .faulty_power = all_faulty_power};
  }

  outcome::result<PowerPair> Deadline::addSectors(
      Runtime &runtime,
      uint64_t partition_size,
      bool proven,
      std::vector<SectorOnChainInfo> sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    if (sectors.empty()) {
      return PowerPair{};
    }

    std::map<ChainEpoch, RleBitset> partition_deadline_update;
    PowerPair activated_power;
    this->live_sectors += sectors.size();
    this->total_sectors += sectors.size();

    {
      OUTCOME_TRY(part_id, this->partitions.size());
      if (part_id > 0) {
        part_id--;  // try filling up the last partition first.
      }

      while (!sectors.empty()) {
        OUTCOME_TRY(maybe_partition, this->partitions.tryGet(part_id));
        Universal<Partition> partition{runtime.getActorVersion()};
        if (maybe_partition) {
          partition = maybe_partition.value();
        } else {
          cbor_blake::cbLoadT(runtime.getIpfsDatastore(), partition);
        }

        const auto sector_count = partition->sectors.size();
        if (sector_count >= partition_size) {
          continue;
        }

        const uint64_t size =
            std::min(partition_size - sector_count, (uint64_t)sectors.size());
        const auto partition_new_sectors = slice(sectors, 0, size);
        sectors = slice(sectors, size);

        OUTCOME_TRY(partition_activated_power,
                    partition->addSectors(
                        runtime, proven, partition_new_sectors, ssize, quant));
        activated_power += partition_activated_power;

        OUTCOME_TRY(this->partitions.set(part_id, partition));

        for (const auto &sector : partition_new_sectors) {
          auto &partition_update = partition_deadline_update[sector.expiration];
          if (!partition_update.empty() && partition_update.has(part_id)) {
            continue;
          }
          partition_update.insert(part_id);
        }

        part_id++;
      }
    }

    {
      BitfieldQueue<kExpirationsBitWidth> deadline_expirations{
          .queue = this->expirations_epochs, .quant = quant};
      OUTCOME_TRY(
          deadline_expirations.addManyToQueueValues(partition_deadline_update));
      this->expirations_epochs = deadline_expirations.queue;
    }

    return activated_power;
  }

  outcome::result<std::tuple<TerminationResult, bool>>
  Deadline::popEarlyTerminations(Runtime &runtime,
                                 uint64_t max_partitions,
                                 uint64_t max_sectors) {
    TerminationResult termination_result;
    std::vector<uint64_t> partitions_finished;

    for (const auto &part_id : this->early_terminations) {
      OUTCOME_TRY(maybe_partition, this->partitions.tryGet(part_id));
      if (!maybe_partition) {
        partitions_finished.push_back(part_id);
        continue;
      }

      auto &partition = maybe_partition.value();

      OUTCOME_TRY(
          result,
          partition->popEarlyTerminations(
              runtime, max_sectors - termination_result.sectors_processed));
      const auto &[partition_result, more] = result;

      termination_result.add(partition_result);

      if (!more) {
        partitions_finished.push_back(part_id);
      }

      OUTCOME_TRY(this->partitions.set(part_id, partition));

      if (!termination_result.belowLimit(max_partitions, max_sectors)) {
        break;
      }
    }

    for (const auto &finished : partitions_finished) {
      this->early_terminations.erase(finished);
    }

    return std::make_tuple(termination_result,
                           !this->early_terminations.empty());
  }

  outcome::result<std::tuple<RleBitset, bool>> Deadline::popExpiredPartitions(
      ChainEpoch until, const QuantSpec &quant) {
    BitfieldQueue<kExpirationsBitWidth> deadline_expirations{
        .queue = this->expirations_epochs, .quant = quant};
    OUTCOME_TRY(result, deadline_expirations.popUntil(until));
    const auto &[popped, modified] = result;

    if (modified) {
      this->expirations_epochs = deadline_expirations.queue;
    }

    return std::make_tuple(popped, modified);
  }

  outcome::result<PowerPair> Deadline::terminateSectors(
      Runtime &runtime,
      const Sectors &sectors,
      ChainEpoch epoch,
      const PartitionSectorMap &partition_sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    PowerPair power_lost;

    for (const auto &[part_id, sector_nos] : partition_sectors.map) {
      OUTCOME_TRY(partition, this->partitions.get(part_id));
      OUTCOME_TRY(removed,
                  partition->terminateSectors(
                      runtime, sectors, epoch, sector_nos, ssize, quant));
      OUTCOME_TRY(this->partitions.set(part_id, partition));

      const auto count = removed.count();
      if (count > 0) {
        this->early_terminations.insert(part_id);
        this->live_sectors -= count;
      }

      this->faulty_power -= removed.faulty_power;
      power_lost += removed.active_power;
    }

    return power_lost;
  }

  outcome::result<std::tuple<RleBitset, RleBitset, PowerPair>>
  Deadline::removePartitions(Runtime &runtime,
                             const RleBitset &to_remove,
                             const QuantSpec &quant) {
    OUTCOME_TRY(partition_count, this->partitions.size());

    if (to_remove.empty()) {
      return std::make_tuple(RleBitset{}, RleBitset{}, PowerPair{});
    }

    for (const auto &part_id : to_remove) {
      if (part_id >= partition_count) {
        return ERROR_TEXT("partition index is out of range");
      }
    }

    if (!this->early_terminations.empty()) {
      return ERROR_TEXT(
          "cannot remove partitions from deadline with early terminations");
    }

    decltype(this->partitions) new_partitions;
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), new_partitions);
    std::vector<RleBitset> all_dead_sectors;
    std::vector<RleBitset> all_live_sectors;
    PowerPair removed_power;

    auto visitor{
        [&](int64_t part_id, const auto &partition) -> outcome::result<void> {
          if (!to_remove.has(part_id)) {
            return new_partitions.append(partition);
          }

          if (!partition->faults.empty()) {
            return ERROR_TEXT("cannot remove, partition has faults");
          }

          if (!partition->unproven.empty()) {
            return ERROR_TEXT("cannot remove, partition has unproven sectors");
          }

          all_dead_sectors.push_back(partition->terminated);
          all_live_sectors.push_back(partition->liveSectors());
          removed_power += partition->live_power;

          return outcome::success();
        }};

    OUTCOME_TRY(this->partitions.visit(visitor));

    this->partitions = new_partitions;

    RleBitset dead;
    dead += all_dead_sectors;

    RleBitset live;
    live += all_live_sectors;

    this->live_sectors -= live.size();
    this->total_sectors -= live.size() + dead.size();

    {
      BitfieldQueue<kExpirationsBitWidth> deadline_expirations{
          .queue = this->expirations_epochs, .quant = quant};
      OUTCOME_TRY(deadline_expirations.cut(to_remove));
      this->expirations_epochs = deadline_expirations.queue;
    }

    return std::make_tuple(live, dead, removed_power);
  }

  outcome::result<void> Deadline::declareFaultsRecovered(
      const Sectors &sectors,
      SectorSize ssize,
      const PartitionSectorMap &partition_sectors) {
    for (const auto &[part_id, sector_nos] : partition_sectors.map) {
      OUTCOME_TRY(partition, this->partitions.get(part_id));
      OUTCOME_TRY(
          partition->declareFaultsRecovered(sectors, ssize, sector_nos));
      OUTCOME_TRY(this->partitions.set(part_id, partition));
    }
    return outcome::success();
  }

  outcome::result<DisputeInfo> Deadline::loadPartitionsForDispute(
      const RleBitset &partition_set) const {
    std::vector<RleBitset> all_sectors;
    std::vector<RleBitset> all_ignored;
    PartitionSectorMap disputed_sectors;
    PowerPair disputed_power;

    for (const auto &part_id : partition_set) {
      OUTCOME_TRY(partition_snapshot, this->partitions_snapshot.get(part_id));

      all_sectors.push_back(partition_snapshot->sectors);
      all_ignored.push_back(partition_snapshot->faults);
      all_ignored.push_back(partition_snapshot->terminated);
      all_ignored.push_back(partition_snapshot->unproven);

      disputed_sectors.add(part_id, partition_snapshot->activeSectors());
      disputed_power += partition_snapshot->activePower();
    }

    RleBitset all_sector_nos;
    all_sector_nos += all_sectors;

    RleBitset all_ignored_nos;
    all_ignored_nos += all_ignored;

    return DisputeInfo{.all_sector_nos = all_sector_nos,
                       .ignored_sector_nos = all_ignored_nos,
                       .disputed_sectors = disputed_sectors,
                       .disputed_power = disputed_power};
  }

  outcome::result<Universal<Deadline>> makeEmptyDeadline(
      const IpldPtr &ipld, const CID &empty_amt_cid) {
    Universal<Deadline> deadline{ipld->actor_version};
    cbor_blake::cbLoadT(ipld, deadline);

    if (ipld->actor_version < ActorVersion::kVersion3) {
      deadline->partitions = {empty_amt_cid, ipld};
      deadline->expirations_epochs = {empty_amt_cid, ipld};
    } else {
      OUTCOME_TRY(empty_partitions_cid, deadline->partitions.amt.flush());
      deadline->partitions_snapshot = {empty_partitions_cid, ipld};

      // Lotus gas conformance - flush empty amt
      OUTCOME_TRY(deadline->expirations_epochs.amt.flush());

      OUTCOME_TRY(empty_post_submissions_cid,
                  deadline->optimistic_post_submissions.amt.flush());
      deadline->optimistic_post_submissions_snapshot = {
          empty_post_submissions_cid, ipld};
    }

    return deadline;
  }

}  // namespace fc::vm::actor::builtin::types::miner
