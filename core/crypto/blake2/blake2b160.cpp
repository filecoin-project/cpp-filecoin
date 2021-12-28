/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/blake2/blake2b160.hpp"

#include <fstream>
#include "common/span.hpp"

#ifndef ROTR64
#define ROTR64(x, y) (((x) >> (y)) ^ ((x) << (64 - (y))))
#endif

#define B2B_GET64(p)                                                        \
  (((uint64_t)((uint8_t *)(p))[0]) ^ (((uint64_t)((uint8_t *)(p))[1]) << 8) \
   ^ (((uint64_t)((uint8_t *)(p))[2]) << 16)                                \
   ^ (((uint64_t)((uint8_t *)(p))[3]) << 24)                                \
   ^ (((uint64_t)((uint8_t *)(p))[4]) << 32)                                \
   ^ (((uint64_t)((uint8_t *)(p))[5]) << 40)                                \
   ^ (((uint64_t)((uint8_t *)(p))[6]) << 48)                                \
   ^ (((uint64_t)((uint8_t *)(p))[7]) << 56))

#define B2B_G(a, b, c, d, x, y)     \
  {                                 \
    v[a] = v[a] + v[b] + (x);       \
    v[d] = ROTR64(v[d] ^ v[a], 32); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR64(v[b] ^ v[c], 24); \
    v[a] = v[a] + v[b] + (y);       \
    v[d] = ROTR64(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR64(v[b] ^ v[c], 63); \
  }

namespace fc::crypto::blake2b {
  static const std::array<uint64_t, 8> iv{
      0x6A09E667F3BCC908,
      0xBB67AE8584CAA73B,
      0x3C6EF372FE94F82B,
      0xA54FF53A5F1D36F1,
      0x510E527FADE682D1,
      0x9B05688C2B3E6C1F,
      0x1F83D9ABFB41BD6B,
      0x5BE0CD19137E2179,
  };
  static const std::array<std::array<uint8_t, 16>, 12> sigma{{
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
      {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
      {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
      {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
      {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
      {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
      {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
      {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
      {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
  }};
  Ctx::Ctx(size_t outlen, BytesIn key) : h{iv}, outlen{outlen} {
    assert(outlen > 0 && outlen <= 64);
    assert(key.size() >= 0 && key.size() <= 64);
    h[0] ^= 0x01010000 ^ (static_cast<size_t>(key.size()) << 8) ^ outlen;
    if (!key.empty()) {
      update(key);
      c = 128;
    }
  }
  void Ctx::update(BytesIn in) {
    for (auto i{0}; i < in.size(); ++i) {
      if (c == 128) {
        t[0] += c;
        if (t[0] < c) {
          t[1]++;
        }
        _compress(false);
        c = 0;
      }
      b[c++] = in[i];
    }
  }
  void Ctx::_compress(bool last) {
    std::array<uint64_t, 16> v{};
    std::array<uint64_t, 16> m{};
    for (auto i{0}; i < 8; ++i) {
      v[i] = h[i];
      v[i + 8] = iv[i];
    }
    v[12] ^= t[0];
    v[13] ^= t[1];
    if (last) {
      v[14] = ~v[14];
    }
    for (auto i{0}; i < 16; ++i) {
      m[i] = B2B_GET64(&b[8 * i]);
    }
    for (auto i{0}; i < 12; ++i) {
      B2B_G(0, 4, 8, 12, m[sigma[i][0]], m[sigma[i][1]]);
      B2B_G(1, 5, 9, 13, m[sigma[i][2]], m[sigma[i][3]]);
      B2B_G(2, 6, 10, 14, m[sigma[i][4]], m[sigma[i][5]]);
      B2B_G(3, 7, 11, 15, m[sigma[i][6]], m[sigma[i][7]]);
      B2B_G(0, 5, 10, 15, m[sigma[i][8]], m[sigma[i][9]]);
      B2B_G(1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
      B2B_G(2, 7, 8, 13, m[sigma[i][12]], m[sigma[i][13]]);
      B2B_G(3, 4, 9, 14, m[sigma[i][14]], m[sigma[i][15]]);
    }
    for (auto i{0}; i < 8; ++i) {
      h[i] ^= v[i] ^ v[i + 8];
    }
  }
  void Ctx::final(gsl::span<uint8_t> hash) {
    assert(hash.size() >= (ssize_t)outlen);
    t[0] += c;
    if (t[0] < c) {
      t[1]++;
    }
    while (c < 128) {
      b[c++] = 0;
    }
    _compress(true);
    for (auto i{0u}; i < outlen; i++) {
      hash[i] = (h[i >> 3] >> (8 * (i & 7))) & 0xFF;
    }
  }

  void hashn(gsl::span<uint8_t> hash, BytesIn input, BytesIn key) {
    Ctx c(hash.size(), key);
    c.update(input);
    return c.final(hash);
  }

  Blake2b160Hash blake2b_160(gsl::span<const uint8_t> to_hash) {
    Blake2b160Hash res{};
    hashn(res, to_hash);
    return res;
  }

  Blake2b256Hash blake2b_256(gsl::span<const uint8_t> to_hash) {
    Blake2b256Hash res{};
    hashn(res, to_hash);
    return res;
  }

  Blake2b512Hash blake2b_512_from_file(std::ifstream &file_stream) {
    if (!file_stream.is_open()) return {};

    Ctx ctx{BLAKE2B512_HASH_LENGTH};

    constexpr size_t buffer_size = 32 * 1024;
    std::string bytes(buffer_size, ' ');
    file_stream.read(bytes.data(), buffer_size);
    auto currently_read = file_stream.gcount();
    while (currently_read != 0) {
      ctx.update(gsl::make_span(common::span::cast<const uint8_t>(bytes.data()),
                                currently_read));
      file_stream.read(bytes.data(), buffer_size);
      currently_read = file_stream.gcount();
    }

    Blake2b512Hash hash;
    ctx.final(hash);
    return hash;
  }

}  // namespace fc::crypto::blake2b
