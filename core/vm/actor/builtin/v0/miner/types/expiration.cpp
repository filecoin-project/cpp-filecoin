/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/types/expiration.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using types::miner::ExpirationSet;

  outcome::result<PowerPair> ExpirationQueue::rescheduleAsFaults(
      ChainEpoch new_expiration,
      const std::vector<SectorOnChainInfo> &sectors,
      SectorSize ssize) {
    RleBitset early_sectors;
    PowerPair expiring_power;
    PowerPair rescheduled_power;

    for (const auto &group :
         groupNewSectorsByDeclaredExpiration(ssize, sectors)) {
      OUTCOME_TRY(es, queue.get(group.epoch));

      if (group.epoch <= quant.quantizeUp(new_expiration)) {
        es.active_power -= group.power;
        es.faulty_power += group.power;
        expiring_power += group.power;
      } else {
        es.on_time_sectors -= group.sectors;
        es.on_time_pledge -= group.pledge;
        es.active_power -= group.power;

        early_sectors += group.sectors;
        rescheduled_power += group.power;
      }

      OUTCOME_TRY(mustUpdateOrDelete(group.epoch, es));
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
            rescheduled_sectors.push_back(es.on_time_sectors);
            rescheduled_sectors.push_back(es.early_sectors);
            rescheduled_power += es.active_power;
            rescheduled_power += es.faulty_power;
          }

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

    for (const auto &group :
         groupNewSectorsByDeclaredExpiration(ssize, sectors)) {
      OUTCOME_TRY(remove(
          group.epoch, group.sectors, {}, group.power, {}, group.pledge));

      removed_snos += group.sectors;
      removed_power += group.power;
      removed_pledge += group.pledge;
    }

    return std::make_tuple(removed_snos, removed_power, removed_pledge);
  }

}  // namespace fc::vm::actor::builtin::v0::miner
