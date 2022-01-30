/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/partition.hpp"

#include "adt/stop.hpp"
#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {

  RleBitset Partition::liveSectors() const {
    return this->sectors - this->terminated;
  }

  outcome::result<std::tuple<RleBitset, PowerPair, PowerPair>>
  Partition::recordFaults(const Sectors &sectors,
                          const RleBitset &sector_nos,
                          ChainEpoch fault_expiration_epoch,
                          SectorSize ssize,
                          const QuantSpec &quant) {
    if (!this->sectors.contains(sector_nos)) {
      return ERROR_TEXT("failed fault declaration");
    }

    const auto retracted_recoveries = recoveries.intersect(sector_nos);
    const auto new_faults =
        sector_nos - retracted_recoveries - this->terminated - this->faults;

    PowerPair new_faulty_power;
    PowerPair power_delta;
    OUTCOME_TRY(new_fault_sectors, sectors.load(new_faults));
    if (!new_fault_sectors.empty()) {
      OUTCOME_TRY(result,
                  addFaults(new_faults,
                            new_fault_sectors,
                            fault_expiration_epoch,
                            ssize,
                            quant));
      std::tie(power_delta, new_faulty_power) = result;
    }

    OUTCOME_TRY(retracted_recovery_sectors, sectors.load(retracted_recoveries));
    if (!retracted_recovery_sectors.empty()) {
      const auto retracted_recovery_power =
          powerForSectors(ssize, retracted_recovery_sectors);
      OUTCOME_TRY(
          removeRecoveries(retracted_recoveries, retracted_recovery_power));
    }

    OUTCOME_TRY(validateState());

    return std::make_tuple(new_faults, power_delta, new_faulty_power);
  }

  outcome::result<PowerPair> Partition::recoverFaults(const Sectors &sectors,
                                                      SectorSize ssize,
                                                      const QuantSpec &quant) {
    OUTCOME_TRY(recovered_sectors, sectors.load(this->recoveries));

    auto queue = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(power, queue->rescheduleRecovered(recovered_sectors, ssize));

    this->expirations_epochs = queue->queue;

    this->faults -= recoveries;
    this->recoveries = {};
    this->faulty_power -= power;
    this->recovering_power -= power;

    OUTCOME_TRY(validateState());

    return std::move(power);
  }

  PowerPair Partition::activateUnproven() {
    const auto new_power = this->unproven_power;
    this->unproven_power = {};
    this->unproven = {};
    return new_power;
  }

  outcome::result<void> Partition::declareFaultsRecovered(
      const Sectors &sectors, SectorSize ssize, const RleBitset &sector_nos) {
    if (!this->sectors.contains(sector_nos)) {
      return ERROR_TEXT("failed fault declaration");
    }

    const auto recoveries =
        sector_nos.intersect(this->faults) - this->recoveries;

    OUTCOME_TRY(recovery_sectors, sectors.load(recoveries));

    this->recoveries += recoveries;
    const auto power = powerForSectors(ssize, recovery_sectors);
    this->recovering_power += power;

    OUTCOME_TRY(validateState());
    return outcome::success();
  }

  outcome::result<void> Partition::removeRecoveries(const RleBitset &sector_nos,
                                                    const PowerPair &power) {
    if (sector_nos.empty()) {
      return outcome::success();
    }

    this->recoveries -= sector_nos;
    this->recovering_power -= power;

    return outcome::success();
  }

  outcome::result<RleBitset> Partition::rescheduleExpirationsV0(
      const Sectors &sectors,
      ChainEpoch new_expiration,
      const RleBitset &sector_nos,
      SectorSize ssize,
      const QuantSpec &quant) {
    const auto present = sector_nos.intersect(this->sectors);
    const auto live = present - this->terminated;
    const auto active = live - this->faults;

    OUTCOME_TRY(sector_infos, sectors.load(active));

    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(expirations->rescheduleExpirations(
        new_expiration, sector_infos, ssize));
    this->expirations_epochs = expirations->queue;

    return active;
  }

  outcome::result<std::vector<Universal<SectorOnChainInfo>>>
  Partition::rescheduleExpirationsV2(const Sectors &sectors,
                                     ChainEpoch new_expiration,
                                     const RleBitset &sector_nos,
                                     SectorSize ssize,
                                     const QuantSpec &quant) {
    const auto present = sector_nos.intersect(this->sectors);
    const auto live = present - this->terminated;
    const auto active = live - this->faults;

    OUTCOME_TRY(sector_infos, sectors.load(active));

    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(expirations->rescheduleExpirations(
        new_expiration, sector_infos, ssize));
    this->expirations_epochs = expirations->queue;

    OUTCOME_TRY(validateState());

    return std::move(sector_infos);
  }

  outcome::result<std::tuple<PowerPair, TokenAmount>> Partition::replaceSectors(
      const std::vector<Universal<SectorOnChainInfo>> &old_sectors,
      const std::vector<Universal<SectorOnChainInfo>> &new_sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(result,
                expirations->replaceSectors(old_sectors, new_sectors, ssize));
    const auto &[old_snos, new_snos, power_delta, pledge_delta] = result;
    this->expirations_epochs = expirations->queue;

    const auto active = activeSectors();

    if (!active.contains(old_snos)) {
      return ERROR_TEXT("refusing to replace inactive sectors");
    }

    this->sectors -= old_snos;
    this->sectors += new_snos;
    this->live_power += power_delta;

    OUTCOME_TRY(validateState());

    return std::make_tuple(power_delta, pledge_delta);
  }

  outcome::result<void> Partition::recordEarlyTermination(
      ChainEpoch epoch, const RleBitset &sectors) {
    BitfieldQueue<kEearlyTerminatedBitWidth> et_queue{
        .queue = this->early_terminated, .quant = kNoQuantization};

    OUTCOME_TRY(et_queue.addToQueue(epoch, sectors));
    this->early_terminated = et_queue.queue;

    return outcome::success();
  }

  outcome::result<std::tuple<PowerPair, PowerPair>>
  Partition::recordMissedPostV0(ChainEpoch fault_expiration,
                                const QuantSpec &quant) {
    auto queue = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(queue->rescheduleAllAsFaults(fault_expiration));
    this->expirations_epochs = queue->queue;

    const auto new_faulty_power = this->live_power - this->faulty_power;
    const auto failed_recovery_power = this->recovering_power;

    this->faults = liveSectors();
    this->recoveries = {};
    this->faulty_power = this->live_power;
    this->recovering_power = {};

    return std::make_tuple(new_faulty_power, failed_recovery_power);
  }

  outcome::result<std::tuple<PowerPair, PowerPair, PowerPair>>
  Partition::recordMissedPostV2(ChainEpoch fault_expiration,
                                const QuantSpec &quant) {
    auto queue = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(queue->rescheduleAllAsFaults(fault_expiration));
    this->expirations_epochs = queue->queue;

    const auto new_faulty_power = this->live_power - this->faulty_power;
    const auto penalized_power = this->recovering_power + new_faulty_power;
    const auto power_delta =
        (new_faulty_power - this->unproven_power).negative();

    this->faults = liveSectors();
    this->recoveries = {};
    this->unproven = {};
    this->faulty_power = this->live_power;
    this->recovering_power = {};
    this->unproven_power = {};

    OUTCOME_TRY(validateState());

    return std::make_tuple(power_delta, penalized_power, new_faulty_power);
  }

  outcome::result<std::tuple<TerminationResult, bool>>
  Partition::popEarlyTerminations(uint64_t max_sectors) {
    BitfieldQueue<kEearlyTerminatedBitWidth> early_terminated_q{
        .queue = this->early_terminated, .quant = kNoQuantization};

    std::vector<uint64_t> processed;
    bool has_remaining = false;
    RleBitset remaining_sectors;
    ChainEpoch remaining_epoch = 0;

    TerminationResult result;
    result.partitions_processed = 1;

    auto visitor{[&](ChainEpoch epoch,
                     const RleBitset &sectors) -> outcome::result<void> {
      auto to_process = sectors;
      const auto count = sectors.size();

      const auto limit = max_sectors - result.sectors_processed;

      if (limit < count) {
        to_process = sectors.slice(0, limit);

        const auto rest = sectors - to_process;

        has_remaining = true;
        remaining_sectors = rest;
        remaining_epoch = epoch;

        result.sectors_processed += limit;
      } else {
        processed.push_back(epoch);
        result.sectors_processed += count;
      }

      result.sectors[epoch] = to_process;

      if (result.sectors_processed >= max_sectors) {
        return outcome::failure(adt::kStopError);
      }

      return outcome::success();
    }};

    CATCH_STOP(early_terminated_q.queue.visit(visitor));

    for (const auto i : processed) {
      OUTCOME_TRY(early_terminated_q.queue.remove(i));
    }

    if (has_remaining) {
      OUTCOME_TRY(
          early_terminated_q.queue.set(remaining_epoch, remaining_sectors));
    }

    this->early_terminated = early_terminated_q.queue;

    OUTCOME_TRY(validateState());

    OUTCOME_TRY(size, early_terminated_q.queue.size());
    return std::make_tuple(result, size > 0);
  }

  outcome::result<std::tuple<PowerPair, PowerPair, PowerPair, bool>>
  Partition::recordSkippedFaults(const Sectors &sectors,
                                 SectorSize ssize,
                                 const QuantSpec &quant,
                                 ChainEpoch fault_expiration,
                                 const RleBitset &skipped) {
    if (skipped.empty()) {
      return std::make_tuple(PowerPair{}, PowerPair{}, PowerPair{}, false);
    }

    if (!this->sectors.contains(skipped)) {
      return VMExitCode::kErrIllegalArgument;
    }

    const auto retracted_recoveries = this->recoveries.intersect(skipped);
    OUTCOME_TRY(retracted_recovery_sectors, sectors.load(retracted_recoveries));
    const auto retracted_recovery_power =
        powerForSectors(ssize, retracted_recovery_sectors);

    const auto new_faults = skipped - this->terminated - this->faults;
    OUTCOME_TRY(new_fault_sectors, sectors.load(new_faults));

    OUTCOME_TRY(
        result,
        addFaults(
            new_faults, new_fault_sectors, fault_expiration, ssize, quant));
    const auto &[power_delta, new_fault_power] = result;

    OUTCOME_TRY(
        removeRecoveries(retracted_recoveries, retracted_recovery_power));

    OUTCOME_TRY(validateState());

    return std::make_tuple(power_delta,
                           new_fault_power,
                           retracted_recovery_power,
                           !new_fault_sectors.empty());
  }

  outcome::result<void> Partition::validatePowerState() const {
    if (this->live_power.raw < 0 || this->live_power.qa < 0) {
      return ERROR_TEXT("Partition left with negative live power");
    }

    if (this->unproven_power.raw < 0 || this->unproven_power.qa < 0) {
      return ERROR_TEXT("Partition left with negative unproven power");
    }

    if (this->faulty_power.raw < 0 || this->faulty_power.qa < 0) {
      return ERROR_TEXT("Partition left with negative faulty power");
    }

    if (this->recovering_power.raw < 0 || this->recovering_power.qa < 0) {
      return ERROR_TEXT("Partition left with negative recovering power");
    }

    if (this->unproven_power.raw > this->live_power.raw) {
      return ERROR_TEXT("Partition left with invalid unproven power");
    }

    if (this->faulty_power.raw > this->live_power.raw) {
      return ERROR_TEXT("Partition left with invalid faulty power");
    }

    if (this->recovering_power.raw > this->live_power.raw
        || this->recovering_power.raw > this->faulty_power.raw) {
      return ERROR_TEXT("Partition left with invalid recovering power");
    }

    return outcome::success();
  }

  outcome::result<void> Partition::validateBFState() const {
    auto merge = this->unproven + this->faults;

    if (this->terminated.containsAny(merge)) {
      return ERROR_TEXT(
          "Partition left with terminated sectors in multiple states");
    }

    merge += this->terminated;

    if (!this->sectors.contains(merge)) {
      return ERROR_TEXT("Partition left with invalid sector state");
    }

    if (!this->faults.contains(this->recoveries)) {
      return ERROR_TEXT("Partition left with invalid recovery state");
    }

    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::types::miner
