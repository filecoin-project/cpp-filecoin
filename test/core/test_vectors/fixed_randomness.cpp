/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/test_vectors/fixed_randomness.hpp"

namespace fc::vm::runtime {
  outcome::result<Randomness> FixedRandomness::getRandomnessFromTickets(
      const TsBranchPtr &ts_branch,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return crypto::randomness::Randomness::fromString(
        "i_am_random_____i_am_random_____");
  }

  outcome::result<Randomness> FixedRandomness::getRandomnessFromBeacon(
      const TsBranchPtr &ts_branch,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return crypto::randomness::Randomness::fromString(
        "i_am_random_____i_am_random_____");
  }

}  // namespace fc::vm::runtime
