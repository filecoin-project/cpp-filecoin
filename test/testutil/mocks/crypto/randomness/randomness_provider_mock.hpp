/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_MOCK_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "filecoin/crypto/randomness/randomness_provider.hpp"

namespace fc::crypto::randomness {

  class MockRandomnessProvider : public RandomnessProvider {
   public:
    MOCK_METHOD2(deriveRandomness,
                 Randomness(DomainSeparationTag tag, Serialization s));
    MOCK_METHOD3(deriveRandomness,
                 Randomness(DomainSeparationTag tag,
                            Serialization s,
                            const ChainEpoch &index));
    MOCK_METHOD3(randomInt,
                 int(const Randomness &randomness, size_t nonce, size_t limit));
  };

}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_MOCK_HPP
