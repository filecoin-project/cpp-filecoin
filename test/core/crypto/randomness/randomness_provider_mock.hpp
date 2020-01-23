/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_MOCK_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "crypto/randomness/randomness_provider.hpp"

namespace fc::crypto::randomness {

  class MockRandomnessProvider : public RandomnessProvider {
   public:
    MOCK_METHOD2(deriveRandomness,
                 Randomness(DomainSeparationTag tag, Serialization s));
    MOCK_METHOD3(deriveRandomness,
                 Randomness(DomainSeparationTag tag,
                            Serialization s,
                            ChainEpoch index));
  };

}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_MOCK_HPP
