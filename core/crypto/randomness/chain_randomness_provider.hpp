/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_CHAIN_RANDOMNESS_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_CHAIN_RANDOMNESS_PROVIDER_HPP

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::crypto::randomness {

  /**
   * @class ChainRandomnessProvider provides method for calculating chain
   * randomness
   */
  class ChainRandomnessProvider {
   public:
    virtual ~ChainRandomnessProvider() = default;

    /** @brief calculate randomness */
    virtual outcome::result<Randomness> sampleRandomness(
        const std::vector<CID> &block_cids, uint64_t round) = 0;
  };
}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_CHAIN_RANDOMNESS_PROVIDER_HPP
