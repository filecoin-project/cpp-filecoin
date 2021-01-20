/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_RANDOMNESS_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_RANDOMNESS_HPP

#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "fwd.hpp"

namespace fc::vm::runtime {

  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using primitives::ChainEpoch;
  using primitives::tipset::TipsetCPtr;

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
        const TipsetCPtr &tipset,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const = 0;

    virtual outcome::result<Randomness> getRandomnessFromBeacon(
        const TipsetCPtr &tipset,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const = 0;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_RANDOMNESS_HPP
