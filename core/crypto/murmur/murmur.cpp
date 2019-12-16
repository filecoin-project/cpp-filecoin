/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/murmur/murmur.hpp"

#include <common/buffer.hpp>

namespace fc::crypto::murmur {
  uint64_t getUint64LE(gsl::span<const uint8_t> bytes) {
    return static_cast<uint64_t>(bytes[0])
           | (static_cast<uint64_t>(bytes[1]) << 8)
           | (static_cast<uint64_t>(bytes[2]) << 16)
           | (static_cast<uint64_t>(bytes[3]) << 24)
           | (static_cast<uint64_t>(bytes[4]) << 32)
           | (static_cast<uint64_t>(bytes[5]) << 40)
           | (static_cast<uint64_t>(bytes[6]) << 48)
           | (static_cast<uint64_t>(bytes[7]) << 56);
  }

  constexpr uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
  }

  inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }

  Hash hash(gsl::span<const uint8_t> input) {
    uint64_t h1 = 0;
    uint64_t h2 = 0;

    const uint64_t c1 = 0x87c37b91114253d5;
    const uint64_t c2 = 0x4cf5ad432745937f;

    constexpr auto kBlockSize = 2 * sizeof(uint64_t);
    auto blocks = input.size() / kBlockSize;

    // blocks
    for (size_t i = 0; i < blocks; ++i) {
      auto k1 = getUint64LE(input.subspan(i * kBlockSize));
      auto k2 = getUint64LE(input.subspan(i * kBlockSize + sizeof(uint64_t)));

      k1 *= c1;
      k1 = rotl64(k1, 31);
      k1 *= c2;
      h1 ^= k1;

      h1 = rotl64(h1, 27);
      h1 += h2;
      h1 = h1 * 5 + 0x52dce729;

      k2 *= c2;
      k2 = rotl64(k2, 33);
      k2 *= c1;
      h2 ^= k2;

      h2 = rotl64(h2, 31);
      h2 += h1;
      h2 = h2 * 5 + 0x38495ab5;
    }

    // tail
    auto tail = input.subspan(blocks * kBlockSize);

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (input.size() & 15) {
      case 15:
        k2 ^= static_cast<uint64_t>(tail[14]) << 48;
      case 14:
        k2 ^= static_cast<uint64_t>(tail[13]) << 40;
      case 13:
        k2 ^= static_cast<uint64_t>(tail[12]) << 32;
      case 12:
        k2 ^= static_cast<uint64_t>(tail[11]) << 24;
      case 11:
        k2 ^= static_cast<uint64_t>(tail[10]) << 16;
      case 10:
        k2 ^= static_cast<uint64_t>(tail[9]) << 8;
      case 9:
        k2 ^= static_cast<uint64_t>(tail[8]) << 0;
        k2 *= c2;
        k2 = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

      case 8:
        k1 ^= static_cast<uint64_t>(tail[7]) << 56;
      case 7:
        k1 ^= static_cast<uint64_t>(tail[6]) << 48;
      case 6:
        k1 ^= static_cast<uint64_t>(tail[5]) << 40;
      case 5:
        k1 ^= static_cast<uint64_t>(tail[4]) << 32;
      case 4:
        k1 ^= static_cast<uint64_t>(tail[3]) << 24;
      case 3:
        k1 ^= static_cast<uint64_t>(tail[2]) << 16;
      case 2:
        k1 ^= static_cast<uint64_t>(tail[1]) << 8;
      case 1:
        k1 ^= static_cast<uint64_t>(tail[0]) << 0;
        k1 *= c1;
        k1 = rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
    };

    // finalization
    h1 ^= input.size();
    h2 ^= input.size();

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;

    auto result_vector = common::Buffer().putUint64(h1).toVector();
    Hash result_array;
    std::move(result_vector.begin(), result_vector.end(), result_array.begin());
    return result_array;
  }
}  // namespace fc::crypto::murmur
