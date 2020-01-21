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
                                ChainEpoch index) override;

    //  TODO: Implement it
    int randomInt(const Randomness &randomness,
                  size_t nonce,
                  size_t limit) override;

   private:
    Randomness deriveRandomnessInternal(DomainSeparationTag tag,
                                        Serialization s,
                                        ChainEpoch index);
  };

}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_RANDOMNESS_PROVIDER_HPP
