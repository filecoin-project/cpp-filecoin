/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/tipset_randomness.hpp"

#include "drand/beaconizer.hpp"
#include "primitives/tipset/chain.hpp"

namespace fc::vm::runtime {
  using drand::BeaconEntry;

  TipsetRandomness::TipsetRandomness(
      TsLoadPtr ts_load,
      SharedMutexPtr ts_branches_mutex,
      std::shared_ptr<DrandSchedule> drand_schedule)
      : ts_load{std::move(ts_load)},
        ts_branches_mutex{std::move(ts_branches_mutex)},
        drand_schedule{std::move(drand_schedule)} {}

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromTickets(
      const TsBranchPtr &ts_branch,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    std::shared_lock ts_lock{*ts_branches_mutex};
    const auto network{version::getNetworkVersion(epoch)};
    OUTCOME_TRY(it,
                find(ts_branch,
                     std::max<ChainEpoch>(0, epoch),
                     network < NetworkVersion::kVersion13));
    OUTCOME_TRY(ts, ts_load->lazyLoad(it.second->second));
    ts_lock.unlock();

    return crypto::randomness::drawRandomness(
        ts->getMinTicketBlock().ticket->bytes, tag, epoch, seed);
  }

  inline outcome::result<BeaconEntry> extractBeaconEntryForEpoch(
      const TsLoadPtr &ts_load,
      primitives::tipset::chain::TsBranchIter it,
      drand::Round round) {
    // magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      OUTCOME_TRY(ts, ts_load->lazyLoad(it.second->second));
      for (const auto &beacon : ts->blks[0].beacon_entries) {
        if (beacon.round == round) {
          return beacon;
        }
      }
      if (it.second->first == 0) {
        break;
      }
      OUTCOME_TRYA(it, stepParent(it));
    }
    return primitives::tipset::TipsetError::kNoBeacons;
  }

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromBeacon(
      const TsBranchPtr &ts_branch,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    std::shared_lock ts_lock{*ts_branches_mutex};
    const auto network{version::getNetworkVersion(epoch)};
    OUTCOME_TRY(it,
                find(ts_branch,
                     std::max<ChainEpoch>(0, epoch),
                     network < NetworkVersion::kVersion13));
    BeaconEntry beacon;
    if (network <= NetworkVersion::kVersion13 || epoch < 0) {
      OUTCOME_TRYA(beacon, latestBeacon(ts_load, it));
    } else {
      OUTCOME_TRYA(beacon,
                   extractBeaconEntryForEpoch(
                       ts_load, it, drand_schedule->maxRound(epoch)));
    }
    ts_lock.unlock();

    return crypto::randomness::drawRandomness(beacon.data, tag, epoch, seed);
  }

}  // namespace fc::vm::runtime
