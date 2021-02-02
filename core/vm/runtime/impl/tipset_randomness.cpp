/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/tipset_randomness.hpp"

#include "primitives/tipset/chain.hpp"

namespace fc::vm::runtime {
  TipsetRandomness::TipsetRandomness(TsLoadPtr ts_load, TsBranchPtr ts_branch)
      : ts_load{std::move(ts_load)}, ts_branch{std::move(ts_branch)} {}

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromTickets(
      const TipsetCPtr &tipset,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    OUTCOME_TRY(it, find(ts_branch, std::max<ChainEpoch>(0, epoch)));
    OUTCOME_TRY(ts, ts_load->load(it.second->second));
    return crypto::randomness::drawRandomness(
        ts->getMinTicketBlock().ticket->bytes, tag, epoch, seed);
  }

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromBeacon(
      const TipsetCPtr &tipset,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    OUTCOME_TRY(
        beacon,
        latestBeacon(ts_load, ts_branch, std::max<ChainEpoch>(0, epoch)));
    return crypto::randomness::drawRandomness(beacon.data, tag, epoch, seed);
  }

}  // namespace fc::vm::runtime
