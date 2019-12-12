/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/blake2/blake2b160.hpp"

#include "crypto/blake2/blake2b.h"

namespace fc::crypto::blake2b {

  fc::outcome::result<Blake2b160Hash> blake2b_160(
      const gsl::span<uint8_t> &to_hash) {
    Blake2b160Hash res{};
    if (::blake2b(res.data(),
                  BLAKE2B160_HASH_LENGHT,
                  nullptr,
                  0,
                  to_hash.data(),
                  to_hash.size())
        != 0)
      return Blake2bError::CANNOT_INIT;
    return res;
  }

}  // namespace fc::crypto::blake2b
