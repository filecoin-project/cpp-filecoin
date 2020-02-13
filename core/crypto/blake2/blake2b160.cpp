/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/blake2/blake2b160.hpp"

#include <iostream>
#include "crypto/blake2/blake2b.h"

namespace fc::crypto::blake2b {

  Blake2b160Hash blake2b_160(gsl::span<const uint8_t> to_hash) {
    Blake2b160Hash res{};
    ::blake2b(res.data(),
              BLAKE2B160_HASH_LENGTH,
              nullptr,
              0,
              to_hash.data(),
              to_hash.size());
    return res;
  }

  Blake2b256Hash blake2b_256(gsl::span<const uint8_t> to_hash) {
    Blake2b256Hash res{};
    ::blake2b(res.data(),
              BLAKE2B256_HASH_LENGTH,
              nullptr,
              0,
              to_hash.data(),
              to_hash.size());
    return res;
  }

  Blake2b512Hash blake2b_512_from_file(std::ifstream &file_stream) {
    if (!file_stream.is_open()) return {};

    blake2b_ctx ctx;

    if (blake2b_init(&ctx, BLAKE2B512_HASH_LENGTH, nullptr, 0)) {
      return {};
    }

    const int one_kb = 1024;
    std::string bytes(one_kb, ' ');
    int bytes_count = file_stream.readsome(bytes.data(), one_kb);
    while (bytes_count != 0) {
      blake2b_update(&ctx, bytes.data(), bytes_count);
      bytes_count = file_stream.readsome(bytes.data(), one_kb);
    }

    Blake2b512Hash hash;
    blake2b_final(&ctx, hash.data());
    return hash;
  }

}  // namespace fc::crypto::blake2b
