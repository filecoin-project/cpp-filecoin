/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "fwd.hpp"

namespace fc::vm::runtime {

  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using primitives::ChainEpoch;

  /**
   * Interface of randomness provider for runtime.
   */
  class RuntimeRandomness {
   public:
    virtual ~RuntimeRandomness() = default;

    /**
     * @brief Returns a (pseudo)random string for the given epoch and tag.
     */
    virtual outcome::result<Randomness> getRandomnessFromTickets(
        const TsBranchPtr &ts_branch,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const = 0;

    virtual outcome::result<Randomness> getRandomnessFromBeacon(
        const TsBranchPtr &ts_branch,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const = 0;
  };

}  // namespace fc::vm::runtime
