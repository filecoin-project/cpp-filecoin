/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_HPP

#include "crypto/randomness/randomness_types.hpp"

namespace fc::crypto::randomness {
  /**
   * @class RandomnessProvider provides 2 methods for drawing randomness value
   */
  class RandomnessProvider {
   public:
    virtual ~RandomnessProvider() = default;

    /**
     * @brief derive randomness value
     * @param tag domain separation tag
     * @param s serialized object bytes
     * @return derived randomness value
     */
    virtual Randomness deriveRandomness(DomainSeparationTag tag,
                                        Serialization s) = 0;

    /**
     * @brief derive randomness value
     * @param tag domain separation tag
     * @param s serialized object bytes
     * @param index epoch index
     * @return derived randomness value
     */
    virtual Randomness deriveRandomness(DomainSeparationTag tag,
                                        Serialization s,
                                        EpochIndex index) = 0;
  };
}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_PROVIDER_HPP
