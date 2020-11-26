/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/tipset_randomness.hpp"

namespace fc::vm::runtime {

  TipsetRandomness::TipsetRandomness(std::shared_ptr<Ipld> ipld,
                                     TipsetCPtr tipset)
      : ipld_{std::move(ipld)}, tipset_{std::move(tipset)} {}

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromTickets(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return tipset_->ticketRandomness(*ipld_, tag, epoch, seed);
  }

  outcome::result<Randomness> TipsetRandomness::getRandomnessFromBeacon(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return tipset_->beaconRandomness(*ipld_, tag, epoch, seed);
  }

}  // namespace fc::vm::runtime
