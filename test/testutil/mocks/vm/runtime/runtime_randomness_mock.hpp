/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "testutil/outcome.hpp"
#include "vm/runtime/runtime_randomness.hpp"

namespace fc::vm::runtime {

  class MockRuntimeRandomness : public RuntimeRandomness {
   public:
    MOCK_CONST_METHOD3(
        getRandomnessFromTickets,
        outcome::result<Randomness>(DomainSeparationTag tag,
                                    ChainEpoch epoch,
                                    gsl::span<const uint8_t> seed));

    MOCK_CONST_METHOD3(
        getRandomnessFromBeacon,
        outcome::result<Randomness>(DomainSeparationTag tag,
                                    ChainEpoch epoch,
                                    gsl::span<const uint8_t> seed));
  };

}  // namespace fc::vm::runtime
