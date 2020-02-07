/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_RANDOMNESS_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_RANDOMNESS_PROVIDER_HPP

#include "crypto/randomness/randomness_provider.hpp"

namespace fc::crypto::randomness {

  class RandomnessProviderImpl : public RandomnessProvider {
   public:
    ~RandomnessProviderImpl() override = default;

    Randomness deriveRandomness(DomainSeparationTag tag,
                                Serialization s) override;

    Randomness deriveRandomness(DomainSeparationTag tag,
                                Serialization s,
                                const ChainEpoch &index) override;

    int randomInt(const Randomness &randomness,
                  size_t nonce,
                  size_t limit) override;

   private:
    /**
     * Internal implementation of random function
     * @param tag - as uint64 - used for seed
     * @param s - used for seed
     * @param index - chain epoch as int64 - used for seed
     * @return Randomness
     */
    Randomness deriveRandomnessInternal(uint64_t tag,
                                        Serialization s,
                                        int64_t index);
  };

}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_RANDOMNESS_PROVIDER_HPP
