/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_TEST_VECTORS_FIXED_RANDOMNESS_HPP
#define CPP_FILECOIN_TEST_CORE_TEST_VECTORS_FIXED_RANDOMNESS_HPP

#include "vm/runtime/runtime_randomness.hpp"

namespace fc::vm::runtime {

  class FixedRandomness : public RuntimeRandomness {
   public:
    outcome::result<Randomness> getRandomnessFromTickets(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    outcome::result<Randomness> getRandomnessFromBeacon(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;
  };

}

#endif  // CPP_FILECOIN_TEST_CORE_TEST_VECTORS_FIXED_RANDOMNESS_HPP
