/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hasher.hpp"

#include "common/error_text.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "crypto/sha/sha256.hpp"

namespace fc::crypto {
  const std::map<Hasher::HashType, Hasher::HashMethod> Hasher::methods_{
      {HashType::sha256, Hasher::sha2_256},
      {HashType::blake2b_256, Hasher::blake2b_256}};

  outcome::result<Hasher::Multihash> Hasher::calculate(
      HashType hash_type, gsl::span<const uint8_t> buffer) {
    const auto it{methods_.find(hash_type)};
    if (it == methods_.end()) {
      return ERROR_TEXT("Hasher::calculate not found");
    }
    return it->second(buffer);
  }

  Hasher::Multihash Hasher::sha2_256(gsl::span<const uint8_t> buffer) {
    auto digest = crypto::sha::sha256(buffer);
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
