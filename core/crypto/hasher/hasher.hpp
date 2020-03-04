/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_CRYPTO_HASHER_HPP
#define FILECOIN_CORE_CRYPTO_HASHER_HPP

#include <map>

#include <libp2p/multi/multihash.hpp>

namespace fc::crypto {
    /**
     * @class Supported methods:
     *        sha2-256
     *        blakeb2-256
     */
  class Hasher {
   protected:
    using HashType = libp2p::multi::HashType;
    using Multihash = libp2p::multi::Multihash;
    using HashMethod = Multihash (*)(gsl::span<const uint8_t>);

   private:
    static std::map<HashType, HashMethod> methods_;

   public:
    static Multihash calculate(HashType hash_type,
                               gsl::span<const uint8_t> buffer);

    /**
     * @brief Calculate SHA2-256 hash
     * @param buffer - source data
     * @return SHA2-256 hash
     */
    static Multihash sha2_256(gsl::span<const uint8_t> buffer);

    /**
     * @brief Calculate Blake2b-256 hash
     * @param buffer - source data
     * @return Blake2b-256 hash
     */
    static Multihash blake2b_256(gsl::span<const uint8_t> buffer);
  };
}  // namespace fc::crypto

#endif
