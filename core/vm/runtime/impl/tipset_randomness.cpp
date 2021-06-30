/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/tipset_randomness.hpp"

#include "primitives/tipset/chain.hpp"

namespace fc::vm::runtime {
  TipsetRandomness::TipsetRandomness(TsLoadPtr ts_load,
                                     SharedMutexPtr ts_branches_mutex)
      : ts_load{std::move(ts_load)},
        ts_branches_mutex{std::move(ts_branches_mutex)} {}

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromTickets(
      const TsBranchPtr &ts_branch,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    std::shared_lock ts_lock{*ts_branches_mutex};
    OUTCOME_TRY(it,
                find(ts_branch,
                     std::max<ChainEpoch>(0, epoch),
                     epoch <= kUpgradeHyperdriveHeight));
    OUTCOME_TRY(ts, ts_load->lazyLoad(it.second->second));
    ts_lock.unlock();

    return crypto::randomness::drawRandomness(
        ts->getMinTicketBlock().ticket->bytes, tag, epoch, seed);
  }

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromBeacon(
      const TsBranchPtr &ts_branch,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    std::shared_lock ts_lock{*ts_branches_mutex};
    OUTCOME_TRY(it,
                find(ts_branch,
                     std::max<ChainEpoch>(0, epoch),
                     epoch <= kUpgradeHyperdriveHeight));
    OUTCOME_TRY(beacon, latestBeacon(ts_load, it));
    ts_lock.unlock();

    return crypto::randomness::drawRandomness(beacon.data, tag, epoch, seed);
  }

}  // namespace fc::vm::runtime
