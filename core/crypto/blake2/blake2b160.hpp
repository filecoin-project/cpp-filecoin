/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BLAKE2B160_HPP
#define CPP_FILECOIN_BLAKE2B160_HPP

#include <gsl/span>

#include "blake2b_error.hpp"
#include "common/outcome.hpp"

namespace fc::crypto::blake2b {

  const size_t BLAKE2B160_HASH_LENGHT = 20; // 160 BIT

  /**
   * @brief Get blake2b-160 hash
   * @param to_hash - data to hash
   * @return outcome with hash or error
   */
  fc::outcome::result<std::array<uint8_t, BLAKE2B160_HASH_LENGHT>> blake2b_160(
      const gsl::span<uint8_t> &to_hash);

}  // namespace fc::crypto::blake2b

#endif  // CPP_FILECOIN_BLAKE2B160_HPP
