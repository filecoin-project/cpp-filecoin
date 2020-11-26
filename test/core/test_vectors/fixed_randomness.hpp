/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_TEST_VECTORS_FIXED_RANDOMNESS_HPP
#define CPP_FILECOIN_TEST_CORE_TEST_VECTORS_FIXED_RANDOMNESS_HPP

#include "vm/runtime/runtime_randomness.hpp"

namespace fc::vm::runtime {

  /**
   * Returns fixed random for conformance tests
   * See lotus implementation
   * https://github.com/filecoin-project/lotus/blob/v0.10.0/conformance/rand_fixed.go
   */
  class FixedRandomness : public RuntimeRandomness {
   public:
    outcome::result<Randomness> getRandomnessFromTickets(
        const TipsetCPtr&,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    outcome::result<Randomness> getRandomnessFromBeacon(
        const TipsetCPtr&,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_TEST_CORE_TEST_VECTORS_FIXED_RANDOMNESS_HPP
