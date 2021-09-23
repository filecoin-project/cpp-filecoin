/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v2/partition.hpp"

#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using types::miner::loadExpirationQueue;
  using types::miner::powerForSectors;
  using types::miner::selectSectors;

  RleBitset Partition::activeSectors() const {
    return liveSectors() - this->faults - this->unproven;
  }

  PowerPair Partition::activePower() const {
    return this->live_power - this->faulty_power - this->unproven_power;
  }

  outcome::result<PowerPair> Partition::addSectors(
      Runtime &runtime,
      bool proven,
      const std::vector<SectorOnChainInfo> &sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);

    RleBitset snos;
    PowerPair power;
    OUTCOME_TRY(result, expirations->addActiveSectors(sectors, ssize));
    std::tie(snos, power, std::ignore) = result;
    this->expirations_epochs = expirations->queue;

    if (this->sectors.containsAny(snos)) {
      return ERROR_TEXT("not all added sectors are new");
    }

    this->sectors += snos;
    this->live_power += power;

    auto proven_power = power;
    if (!proven) {
      this->unproven_power += power;
      this->unproven += snos;
      proven_power = {};
    }

    OUTCOME_TRY(validateState());

    return proven_power;
  }

  outcome::result<std::tuple<PowerPair, PowerPair>> Partition::addFaults(
      Runtime &runtime,
      const RleBitset &sector_nos,
      const std::vector<SectorOnChainInfo> &sectors,
      ChainEpoch fault_expiration,
      SectorSize ssize,
      const QuantSpec &quant) {
    auto queue = loadExpirationQueue(this->expirations_epochs, quant);

    OUTCOME_TRY(new_faulty_power,
                queue->rescheduleAsFaults(fault_expiration, sectors, ssize));
    this->expirations_epochs = queue->queue;

    this->faults += sector_nos;
    this->faulty_power += new_faulty_power;

    const auto unproven = sector_nos.intersect(this->unproven);
    this->unproven -= unproven;

    auto power_delta = new_faulty_power.negative();

    OUTCOME_TRY(unproven_infos, selectSectors(sectors, unproven));
    if (!unproven_infos.empty()) {
      const auto lost_unproven_power = powerForSectors(ssize, unproven_infos);
      this->unproven_power -= lost_unproven_power;
      power_delta += lost_unproven_power;
    }

    OUTCOME_TRY(validateState());

    return std::make_tuple(power_delta, new_faulty_power);
  }

  outcome::result<ExpirationSet> Partition::terminateSectors(
      Runtime &runtime,
      const Sectors &sectors,
      ChainEpoch epoch,
      const RleBitset &sector_nos,
      SectorSize ssize,
      const QuantSpec &quant) {
    const auto live_sectors = liveSectors();

    if (!live_sectors.contains(sector_nos)) {
      return ERROR_TEXT("can only terminate live sectors");
    }

    OUTCOME_TRY(sector_infos, sectors.load(sector_nos));
    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(result,
                expirations->removeSectors(
                    sector_infos, this->faults, this->recoveries, ssize));
    auto &[removed, removed_recovering] = result;
    this->expirations_epochs = expirations->queue;

    const auto removed_sectors =
        removed.on_time_sectors + removed.early_sectors;
    OUTCOME_TRY(recordEarlyTermination(runtime, epoch, removed_sectors));

    const auto unproven_nos = removed_sectors.intersect(this->unproven);

    this->faults -= removed_sectors;
    this->recoveries -= removed_sectors;
    this->terminated += removed_sectors;
    this->unproven -= unproven_nos;

    this->live_power =
        this->live_power - removed.active_power - removed.faulty_power;
    this->faulty_power -= removed.faulty_power;
    this->recovering_power -= removed_recovering;

    OUTCOME_TRY(unproven_infos, selectSectors(sector_infos, unproven_nos));
    const auto removed_unproven_power = powerForSectors(ssize, unproven_infos);
    this->unproven_power -= removed_unproven_power;
    removed.active_power -= removed_unproven_power;

    OUTCOME_TRY(validateState());

    return removed;
  }

  outcome::result<ExpirationSet> Partition::popExpiredSectors(
      Runtime &runtime, ChainEpoch until, const QuantSpec &quant) {
    if (!this->unproven.empty()) {
      return ERROR_TEXT(
          "cannot pop expired sectors from a partition with unproven sectors");
    }

    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);
    OUTCOME_TRY(popped, expirations->popUntil(until));
    this->expirations_epochs = expirations->queue;

    const auto expired_sectors = popped.on_time_sectors + popped.early_sectors;

    if (!this->recoveries.empty()) {
      return ERROR_TEXT("unexpected recoveries while processing expirations");
    }

    if (!this->recovering_power.isZero()) {
      return ERROR_TEXT(
          "unexpected recovering power while processing expirations");
    }

    if (this->terminated.containsAny(expired_sectors)) {
      return ERROR_TEXT("expiring sectors already terminated");
    }

    this->terminated += expired_sectors;
    this->faults -= expired_sectors;
    this->live_power -= popped.active_power + popped.faulty_power;
    this->faulty_power -= popped.faulty_power;

    OUTCOME_TRY(recordEarlyTermination(runtime, until, popped.early_sectors));

    OUTCOME_TRY(validateState());

    return std::move(popped);
  }

  outcome::result<void> Partition::validateState() const {
    OUTCOME_TRY(validatePowerState());
    OUTCOME_TRY(validateBFState());
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v2::miner
