/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v2/expiration.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using types::miner::SectorEpochSet;

  outcome::result<PowerPair> ExpirationQueue::rescheduleAsFaults(
      ChainEpoch new_expiration,
      const std::vector<SectorOnChainInfo> &sectors,
      SectorSize ssize) {
    RleBitset early_sectors;
    PowerPair expiring_power;
    PowerPair rescheduled_power;

    OUTCOME_TRY(groups, findSectorsByExpiration(ssize, sectors));

    for (auto group : groups) {
      if (group.sector_epoch_set.epoch <= quant.quantizeUp(new_expiration)) {
        group.es.active_power -= group.sector_epoch_set.power;
        group.es.faulty_power += group.sector_epoch_set.power;
        expiring_power += group.sector_epoch_set.power;
      } else {
        group.es.on_time_sectors -= group.sector_epoch_set.sectors;
        group.es.on_time_pledge -= group.sector_epoch_set.pledge;
        group.es.active_power -= group.sector_epoch_set.power;

        early_sectors += group.sector_epoch_set.sectors;
        rescheduled_power += group.sector_epoch_set.power;
      }

      OUTCOME_TRY(mustUpdateOrDelete(group.sector_epoch_set.epoch, group.es));

      OUTCOME_TRY(group.es.validateState());
    }

    if (!early_sectors.empty()) {
      OUTCOME_TRY(
          add(new_expiration, {}, early_sectors, {}, rescheduled_power, 0));
    }

    return rescheduled_power + expiring_power;
  }

  outcome::result<void> ExpirationQueue::rescheduleAllAsFaults(
      ChainEpoch fault_expiration) {
    std::vector<uint64_t> rescheduled_epochs;
    std::vector<RleBitset> rescheduled_sectors;
    PowerPair rescheduled_power;

    auto visitor{
        [&](ChainEpoch epoch, ExpirationSet es) -> outcome::result<void> {
          if (epoch <= quant.quantizeUp(fault_expiration)) {
            es.faulty_power += es.active_power;
            es.active_power = PowerPair();
            OUTCOME_TRY(queue.set(epoch, es));
          } else {
            rescheduled_epochs.push_back(epoch);

            if (!es.early_sectors.empty()) {
              return ERROR_TEXT(
                  "attempted to re-schedule early expirations to an even "
                  "earlier epoch");
            }
            rescheduled_sectors.push_back(es.on_time_sectors);

            rescheduled_power += es.active_power;
            rescheduled_power += es.faulty_power;
          }

          OUTCOME_TRY(es.validateState());

          return outcome::success();
        }};

    OUTCOME_TRY(queue.visit(visitor));

    if (rescheduled_epochs.empty()) {
      return outcome::success();
    }

    RleBitset all_rescheduled;
    all_rescheduled += rescheduled_sectors;

    OUTCOME_TRY(
        add(fault_expiration, {}, all_rescheduled, {}, rescheduled_power, 0));

    for (const auto epoch : rescheduled_epochs) {
      OUTCOME_TRY(queue.remove(epoch));
    }

    return outcome::success();
  }

  outcome::result<std::tuple<RleBitset, PowerPair, TokenAmount>>
  ExpirationQueue::removeActiveSectors(
      const std::vector<SectorOnChainInfo> &sectors, SectorSize ssize) {
    RleBitset removed_snos;
    PowerPair removed_power;
    TokenAmount removed_pledge{0};

    OUTCOME_TRY(groups, findSectorsByExpiration(ssize, sectors));

    for (const auto &group : groups) {
      OUTCOME_TRY(remove(group.sector_epoch_set.epoch,
                         group.sector_epoch_set.sectors,
                         {},
                         group.sector_epoch_set.power,
                         {},
                         group.sector_epoch_set.pledge));

      removed_snos += group.sector_epoch_set.sectors;
      removed_power += group.sector_epoch_set.power;
      removed_pledge += group.sector_epoch_set.pledge;
    }

    return std::make_tuple(removed_snos, removed_power, removed_pledge);
  }

  outcome::result<std::vector<SectorExpirationSet>>
  ExpirationQueue::findSectorsByExpiration(
      SectorSize ssize, const std::vector<SectorOnChainInfo> &sectors) {
    std::set<ChainEpoch> declared_expirations;
    std::map<SectorNumber, SectorOnChainInfo> sectors_by_number;
    RleBitset all_remaining;
    std::vector<SectorExpirationSet> expiration_groups;

    for (const auto &sector : sectors) {
      const auto q_expiration = quant.quantizeUp(sector.expiration);
      declared_expirations.insert(q_expiration);
      all_remaining.insert(sector.sector);
      sectors_by_number[sector.sector] = sector;
    }

    for (const auto &expiration : declared_expirations) {
      OUTCOME_TRY(es, queue.get(expiration));

      SectorExpirationSet group;
      std::tie(group, all_remaining) = groupExpirationSet(
          ssize, sectors_by_number, all_remaining, es, expiration);

      if (!group.sector_epoch_set.sectors.empty()) {
        expiration_groups.push_back(group);
      }
    }

    if (!all_remaining.empty()) {
      MutateFunction f{[&](ChainEpoch epoch, const ExpirationSet &es)
                           -> outcome::result<std::tuple<bool, bool>> {
        const auto found = declared_expirations.find(epoch);
        if (found != declared_expirations.end()) {
          return std::tuple(false, true);
        }

        for (const auto &u : es.early_sectors) {
          if (all_remaining.has(u)) {
            return std::make_tuple(false, true);
          }
        }

        SectorExpirationSet group;
        std::tie(group, all_remaining) = groupExpirationSet(
            ssize, sectors_by_number, all_remaining, es, epoch);

        if (!group.sector_epoch_set.sectors.empty()) {
          expiration_groups.push_back(group);
        }

        return std::make_tuple(false, !all_remaining.empty());
      }};

      OUTCOME_TRY(traverseMutate(f));
    }

    if (!all_remaining.empty()) {
      return ERROR_TEXT("some sectors not found in expiration queue");
    }

    std::sort(
        expiration_groups.begin(),
        expiration_groups.end(),
        [](const SectorExpirationSet &lhs, const SectorExpirationSet &rhs) {
          return lhs.sector_epoch_set.epoch < rhs.sector_epoch_set.epoch;
        });

    return expiration_groups;
  }

  std::tuple<SectorExpirationSet, RleBitset>
  ExpirationQueue::groupExpirationSet(
      SectorSize ssize,
      const std::map<SectorNumber, SectorOnChainInfo> &sectors,
      RleBitset &include_set,
      const ExpirationSet &es,
      ChainEpoch expiration) {
    RleBitset sector_numbers;
    PowerPair total_power;
    TokenAmount total_pledge{0};

    for (const auto &u : es.on_time_sectors) {
      if (include_set.has(u)) {
        const auto &sector = sectors.at(u);
        sector_numbers.insert(u);
        total_power +=
            PowerPair(ssize, types::miner::qaPowerForSector(ssize, sector));
        total_pledge += sector.init_pledge;
        include_set.erase(u);
      }
    }

    SectorExpirationSet ses{
        .sector_epoch_set = SectorEpochSet{.epoch = expiration,
                                           .sectors = sector_numbers,
                                           .power = total_power,
                                           .pledge = total_pledge},
        .es = es};

    return std::make_tuple(ses, include_set);
  }

}  // namespace fc::vm::actor::builtin::v2::miner
