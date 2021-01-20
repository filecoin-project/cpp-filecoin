/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/tipset_randomness.hpp"

#include "primitives/tipset/tipset.hpp"

namespace fc::vm::runtime {

  TipsetRandomness::TipsetRandomness(std::shared_ptr<Ipld> ipld)
      : ipld_{std::move(ipld)} {}

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromTickets(
      const TipsetCPtr &tipset,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return tipset->ticketRandomness(*ipld_, tag, epoch, seed);
  }

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromBeacon(
      const TipsetCPtr &tipset,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return tipset->beaconRandomness(*ipld_, tag, epoch, seed);
  }

}  // namespace fc::vm::runtime
