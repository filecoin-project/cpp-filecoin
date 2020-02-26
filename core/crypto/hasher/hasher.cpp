/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hasher.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include "crypto/blake2/blake2b160.hpp"

namespace fc::crypto {
  std::map<Hasher::HashType, Hasher::HashMethod> Hasher::methods_{
      {HashType::sha256, Hasher::sha2_256},
      {HashType::blake2b_256, Hasher::blake2b_256}};

  Hasher::Multihash Hasher::calculate(HashType hash_type,
                                      gsl::span<const uint8_t> buffer) {
    HashMethod hash_method = methods_.at(hash_type);
    return std::invoke(hash_method, buffer);
  }

  Hasher::Multihash Hasher::sha2_256(gsl::span<const uint8_t> buffer) {
    auto digest = libp2p::crypto::sha256(buffer);
    auto multi_hash = Multihash::create(HashType::sha256, digest);
    BOOST_ASSERT_MSG(multi_hash.has_value(),
                     "fc::crypto::Hasher - failed to create sha2-256 hash");
    return multi_hash.value();
  }

  Hasher::Multihash Hasher::blake2b_256(gsl::span<const uint8_t> buffer) {
    auto digest = crypto::blake2b::blake2b_256(buffer);
    auto multi_hash = Multihash::create(HashType::blake2b_256, digest);
    BOOST_ASSERT_MSG(multi_hash.has_value(),
                     "fc::crypto::Hasher - failed to create blake2b_256 hash");
    return multi_hash.value();
  }
}  // namespace fc::crypto
