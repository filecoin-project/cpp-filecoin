/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_CHAIN_RANDOMNESS_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_CHAIN_RANDOMNESS_PROVIDER_IMPL_HPP

#include "crypto/randomness/chain_randomness_provider.hpp"

namespace fc::storage::blockchain {
  class ChainStore;
}

namespace fc::crypto::randomness {
  class ChainRandomnessProviderImpl : public ChainRandomnessProvider {
   public:
    ~ChainRandomnessProviderImpl() override = default;

    explicit ChainRandomnessProviderImpl(
        std::shared_ptr<storage::blockchain::ChainStore> chain_store);

    outcome::result<Randomness> sampleRandomness(const std::vector<CID> &blks,
                                                 uint64_t round) override;

   private:
    std::shared_ptr<storage::blockchain::ChainStore> chain_store_;
  };
}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_IMPL_CHAIN_RANDOMNESS_PROVIDER_IMPL_HPP
