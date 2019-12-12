/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BLAKE2B160_HPP
#define CPP_FILECOIN_BLAKE2B160_HPP

#include <gsl/span>

#include "common/blob.hpp"
#include "common/outcome.hpp"
#include "crypto/blake2/blake2b_error.hpp"

namespace fc::crypto::blake2b {

  const size_t BLAKE2B160_HASH_LENGHT = 20;  // 160 BIT

  using Blake2b160Hash = common::Blob<BLAKE2B160_HASH_LENGHT>;

  /**
   * @brief Get blake2b-160 hash
   * @param to_hash - data to hash
   * @return outcome with hash or error
   */
  fc::outcome::result<Blake2b160Hash> blake2b_160(
      gsl::span<const uint8_t> to_hash);

}  // namespace fc::crypto::blake2b

#endif  // CPP_FILECOIN_BLAKE2B160_HPP
