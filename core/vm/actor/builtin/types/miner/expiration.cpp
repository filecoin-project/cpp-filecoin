/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::SectorNumber;

  // ExpirationSet
  //============================================================================

  outcome::result<void> ExpirationSet::add(const RleBitset &on_time_sectors,
                                           const RleBitset &early_sectors,
                                           const TokenAmount &on_time_pledge,
                                           const PowerPair &active_power,
                                           const PowerPair &faulty_power) {
    this->on_time_sectors += on_time_sectors;
    this->early_sectors += early_sectors;
    this->on_time_pledge += on_time_pledge;
    this->active_power += active_power;
    this->faulty_power += faulty_power;
    return validateState();
  }

  outcome::result<void> ExpirationSet::remove(const RleBitset &on_time_sectors,
                                              const RleBitset &early_sectors,
                                              const TokenAmount &on_time_pledge,
                                              const PowerPair &active_power,
                                              const PowerPair &faulty_power) {
    if (!this->on_time_sectors.contains(on_time_sectors)) {
      return ERROR_TEXT("removing on-time sectors that are not contained");
    }
    if (!this->early_sectors.contains(early_sectors)) {
      return ERROR_TEXT("removing early sectors that are not contained");
    }

    this->on_time_sectors -= on_time_sectors;
    this->early_sectors -= early_sectors;
    this->on_time_pledge -= on_time_pledge;
    this->active_power -= active_power;
    this->faulty_power -= faulty_power;

    if (this->on_time_pledge < 0) {
      return ERROR_TEXT("expiration set pledge underflow");
    }
    if (this->active_power.qa < 0 || this->faulty_power.qa < 0) {
      return ERROR_TEXT("expiration set power underflow");
    }

    return validateState();
  }

  bool ExpirationSet::isEmpty() const {
    return on_time_sectors.empty() && early_sectors.empty();
  }

  uint64_t ExpirationSet::count() const {
    return on_time_sectors.size() + early_sectors.size();
  }

  outcome::result<void> ExpirationSet::validateState() const {
    static const auto kError = ERROR_TEXT("ExpirationSet has negative field");

    if (on_time_pledge < 0) {
      return kError;
    }
    if (active_power.raw < 0) {
      return kError;
    }
    if (active_power.qa < 0) {
      return kError;
    }
    if (faulty_power.raw < 0) {
      return kError;
    }
    if (faulty_power.qa < 0) {
      return kError;
    }

    return outcome::success();
  }

  // ExpirationQueue
  //============================================================================

  outcome::result<std::tuple<RleBitset, PowerPair, TokenAmount>>
  ExpirationQueue::addActiveSectors(
      const std::vector<SectorOnChainInfo> &sectors, SectorSize ssize) {
    PowerPair total_power;
    TokenAmount total_pledge{0};
    std::vector<RleBitset> total_sectors;

    for (const auto &group :
         groupNewSectorsByDeclaredExpiration(ssize, sectors)) {
      OUTCOME_TRY(
          add(group.epoch, group.sectors, {}, group.power, {}, group.pledge));
      total_sectors.push_back(group.sectors);
      total_power += group.power;
      total_pledge += group.pledge;
    }

    RleBitset snos;
    snos += total_sectors;

    return std::make_tuple(snos, total_power, total_pledge);
  }

  outcome::result<void> ExpirationQueue::rescheduleExpirations(
      ChainEpoch new_expiration,
      const std::vector<SectorOnChainInfo> &sectors,
      SectorSize ssize) {
    if (sectors.empty()) {
      return outcome::success();
    }

    OUTCOME_TRY(result, removeActiveSectors(sectors, ssize));
    const auto &[snos, power, pledge] = result;

    OUTCOME_TRY(add(new_expiration, snos, {}, power, {}, pledge));

    return outcome::success();
  }

  outcome::result<PowerPair> ExpirationQueue::rescheduleRecovered(
      const std::vector<SectorOnChainInfo> &sectors, SectorSize ssize) {
    std::vector<SectorNumber> remaining;
    for (const auto &sector : sectors) {
      remaining.push_back(sector.sector);
    }

    std::vector<SectorOnChainInfo> sectors_rescheduled;
    PowerPair recovered_power;

    MutateFunction f{
        [&](ChainEpoch epoch,
            ExpirationSet &es) -> outcome::result<std::tuple<bool, bool>> {
          bool changed = false;

          for (const auto &sector : sectors) {
            const PowerPair power(ssize, qaPowerForSector(ssize, sector));
            bool found = false;

            if (es.on_time_sectors.has(sector.sector)) {
              found = true;
              // If the sector expires on-time at this epoch, leave it here but
              // change faulty power to active. The pledge is already part of
              // the on-time pledge at this entry.
              es.faulty_power -= power;
              es.active_power += power;
            } else if (es.early_sectors.has(sector.sector)) {
              found = true;
              // If the sector expires early at this epoch, remove it for
              // re-scheduling. It's not part of the on-time pledge number here.
              es.early_sectors.unset(sector.sector);
              es.faulty_power -= power;
              sectors_rescheduled.push_back(sector);
            }

            if (found) {
              recovered_power += power;

              const auto remove_it =
                  std::find(remaining.begin(), remaining.end(), sector.sector);
              if (remove_it != remaining.end()) {
                remaining.erase(remove_it);
              }

              changed = true;
            }
          }

          OUTCOME_TRY(es.validateState());

          return std::make_tuple(changed, !remaining.empty());
        }};

    OUTCOME_TRY(traverseMutate(f));

    if (!remaining.empty()) {
      return ERROR_TEXT("sectors not found in expiration queue");
    }

    OUTCOME_TRY(addActiveSectors(sectors_rescheduled, ssize));
    return recovered_power;
  }

  outcome::result<std::tuple<RleBitset, RleBitset, PowerPair, TokenAmount>>
  ExpirationQueue::replaceSectors(
      const std::vector<SectorOnChainInfo> &old_sectors,
      const std::vector<SectorOnChainInfo> &new_sectors,
      SectorSize ssize) {
    OUTCOME_TRY(remove_result, removeActiveSectors(old_sectors, ssize));
    const auto &[old_snos, old_power, old_pledge] = remove_result;

    OUTCOME_TRY(add_result, addActiveSectors(new_sectors, ssize));
    const auto &[new_snos, new_power, new_pledge] = add_result;

    return std::make_tuple(
        old_snos, new_snos, new_power - old_power, new_pledge - old_pledge);
  }

  outcome::result<std::tuple<ExpirationSet, PowerPair>>
  ExpirationQueue::removeSectors(const std::vector<SectorOnChainInfo> &sectors,
                                 const RleBitset &faults,
                                 const RleBitset &recovering,
                                 SectorSize ssize) {
    std::vector<SectorNumber> remaining;
    for (const auto &sector : sectors) {
      remaining.push_back(sector.sector);
    }

    ExpirationSet removed;
    PowerPair recovering_power;

    std::vector<SectorOnChainInfo> non_faulty_sectors;
    std::vector<SectorOnChainInfo> faulty_sectors;

    for (const auto &sector : sectors) {
      if (faults.has(sector.sector)) {
        faulty_sectors.push_back(sector);
        continue;
      }
      non_faulty_sectors.push_back(sector);

      const auto remove_it =
          std::find(remaining.begin(), remaining.end(), sector.sector);
      if (remove_it != remaining.end()) {
        remaining.erase(remove_it);
      }
    }

    OUTCOME_TRY(result, removeActiveSectors(non_faulty_sectors, ssize));
    std::tie(
        removed.on_time_sectors, removed.active_power, removed.on_time_pledge) =
        result;

    MutateFunction f{
        [&](ChainEpoch epoch,
            ExpirationSet &es) -> outcome::result<std::tuple<bool, bool>> {
          bool changed = false;

          for (const auto &sector : faulty_sectors) {
            bool found = false;

            if (es.on_time_sectors.has(sector.sector)) {
              found = true;
              es.on_time_sectors.unset(sector.sector);
              removed.on_time_sectors.insert(sector.sector);
              es.on_time_pledge -= sector.init_pledge;
              removed.on_time_pledge += sector.init_pledge;
            } else if (es.early_sectors.has(sector.sector)) {
              found = true;
              es.early_sectors.unset(sector.sector);
              removed.early_sectors.insert(sector.sector);
            }

            if (found) {
              const PowerPair power(ssize, qaPowerForSector(ssize, sector));

              if (faults.has(sector.sector)) {
                es.faulty_power -= power;
                removed.faulty_power += power;
              } else {
                es.active_power -= power;
                removed.active_power += power;
              }

              if (recovering.has(sector.sector)) {
                recovering_power += power;
              }

              const auto remove_it =
                  std::find(remaining.begin(), remaining.end(), sector.sector);
              if (remove_it != remaining.end()) {
                remaining.erase(remove_it);
              }

              changed = true;
            }
          }

          OUTCOME_TRY(es.validateState());

          return std::make_tuple(changed, !remaining.empty());
        }};

    OUTCOME_TRY(traverseMutate(f));

    if (!remaining.empty()) {
      return ERROR_TEXT("sectors not found in expiration queue");
    }

    return std::make_tuple(removed, recovering_power);
  }

  outcome::result<ExpirationSet> ExpirationQueue::popUntil(ChainEpoch until) {
    std::vector<RleBitset> on_time_sectors;
    std::vector<RleBitset> early_sectors;
    PowerPair active_power;
    PowerPair faulty_power;
    TokenAmount on_time_pledge{0};
    std::vector<uint64_t> popped_keys;

    auto visitor{[&](ChainEpoch epoch,
                     const ExpirationSet &es) -> outcome::result<void> {
      if (epoch > until) {
        return outcome::success();
      }
      popped_keys.push_back(epoch);
      on_time_sectors.push_back(es.on_time_sectors);
      early_sectors.push_back(es.early_sectors);
      active_power += es.active_power;
      faulty_power += es.faulty_power;
      on_time_pledge += es.on_time_pledge;
      return outcome::success();
    }};

    OUTCOME_TRY(queue.visit(visitor));

    for (const auto epoch : popped_keys) {
      OUTCOME_TRY(queue.remove(epoch));
    }

    RleBitset all_on_time;
    all_on_time += on_time_sectors;

    RleBitset all_early;
    all_early += early_sectors;

    return ExpirationSet{.on_time_sectors = all_on_time,
                         .early_sectors = all_early,
                         .on_time_pledge = on_time_pledge,
                         .active_power = active_power,
                         .faulty_power = faulty_power};
  }

  outcome::result<void> ExpirationQueue::add(ChainEpoch raw_epoch,
                                             const RleBitset &on_time_sectors,
                                             const RleBitset &early_sectors,
                                             const PowerPair &active_power,
                                             const PowerPair &faulty_power,
                                             const TokenAmount &pledge) {
    const auto epoch = quant.quantizeUp(raw_epoch);
    ExpirationSet es;
    OUTCOME_TRY(maybe_es, queue.tryGet(epoch));
    if (maybe_es) {
      es = maybe_es.value();
    }
    OUTCOME_TRY(es.add(
        on_time_sectors, early_sectors, pledge, active_power, faulty_power));
    OUTCOME_TRY(queue.set(epoch, es));
    return outcome::success();
  }

  outcome::result<void> ExpirationQueue::remove(
      ChainEpoch raw_epoch,
      const RleBitset &on_time_sectors,
      const RleBitset &early_sectors,
      const PowerPair &active_power,
      const PowerPair &faulty_power,
      const TokenAmount &pledge) {
    const auto epoch = quant.quantizeUp(raw_epoch);
    OUTCOME_TRY(es, queue.get(epoch));
    OUTCOME_TRY(es.remove(
        on_time_sectors, early_sectors, pledge, active_power, faulty_power));
    return mustUpdateOrDelete(epoch, es);
  }

  outcome::result<void> ExpirationQueue::traverseMutate(MutateFunction f) {
    ExpirationSet exp_set;
    std::vector<uint64_t> epochs_emptied;

    bool stop = false;

    auto visitor{
        [&](ChainEpoch epoch, ExpirationSet es) -> outcome::result<void> {
          if (stop) {
            return outcome::success();
          }

          OUTCOME_TRY(result, f(epoch, es));
          const auto &[changed, keep_going] = result;

          if (changed) {
            if (es.isEmpty()) {
              epochs_emptied.push_back(epoch);
            } else {
              OUTCOME_TRY(queue.set(epoch, es));
            }
          }

          if (!keep_going) {
            stop = true;
          }
          return outcome::success();
        }};

    OUTCOME_TRY(queue.visit(visitor));

    for (const auto epoch : epochs_emptied) {
      OUTCOME_TRY(queue.remove(epoch));
    }

    return outcome::success();
  }

  outcome::result<void> ExpirationQueue::mustUpdateOrDelete(
      ChainEpoch epoch, const ExpirationSet &es) {
    if (es.isEmpty()) {
      OUTCOME_TRY(queue.remove(epoch));
    } else {
      OUTCOME_TRY(queue.set(epoch, es));
    }
    return outcome::success();
  }

  std::vector<SectorEpochSet>
  ExpirationQueue::groupNewSectorsByDeclaredExpiration(
      SectorSize sector_size,
      const std::vector<SectorOnChainInfo> &sectors) const {
    std::map<ChainEpoch, std::vector<SectorOnChainInfo>> sectors_by_expiration;

    for (const auto &sector : sectors) {
      const auto q_expiration = quant.quantizeUp(sector.expiration);
      sectors_by_expiration[q_expiration].push_back(sector);
    }

    std::vector<SectorEpochSet> sector_epoch_sets;

    for (const auto &[expiration, epoch_sectors] : sectors_by_expiration) {
      RleBitset sector_numbers;
      PowerPair total_power;
      TokenAmount total_pledge{0};
      for (size_t i = 0; i < epoch_sectors.size(); i++) {
        sector_numbers.insert(epoch_sectors[i].sector);
        total_power += PowerPair(
            sector_size, qaPowerForSector(sector_size, epoch_sectors[i]));
        total_pledge += epoch_sectors[i].init_pledge;
      }

      sector_epoch_sets.push_back(SectorEpochSet{.epoch = expiration,
                                                 .sectors = sector_numbers,
                                                 .power = total_power,
                                                 .pledge = total_pledge});
    }

    std::sort(sector_epoch_sets.begin(),
              sector_epoch_sets.end(),
              [](const SectorEpochSet &lhs, const SectorEpochSet &rhs) {
                return lhs.epoch < rhs.epoch;
              });
    return sector_epoch_sets;
  }

}  // namespace fc::vm::actor::builtin::types::miner
