/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BLAKE2B160_HPP
#define CPP_FILECOIN_BLAKE2B160_HPP

#include <iosfwd>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace fc::crypto::blake2b {

  const size_t BLAKE2B160_HASH_LENGTH = 20;  // 160 BIT
  const size_t BLAKE2B256_HASH_LENGTH = 32;  // 256 BIT
  const size_t BLAKE2B512_HASH_LENGTH = 64;  // 512 BIT

  using Blake2b160Hash = common::Blob<BLAKE2B160_HASH_LENGTH>;
  using Blake2b256Hash = common::Blob<BLAKE2B256_HASH_LENGTH>;
  using Blake2b512Hash = common::Blob<BLAKE2B512_HASH_LENGTH>;

  struct Ctx {
    Ctx(size_t outlen, BytesIn key = {});
    void update(BytesIn in);
    void _compress(bool last);
    void final(gsl::span<uint8_t> hash);

    uint8_t b[128]{};
    uint64_t h[8];
    uint64_t t[2]{};
    size_t c{};
    size_t outlen;
  };

  void hashn(gsl::span<uint8_t> hash, BytesIn input, BytesIn key = {});

  /**
   * @brief Get blake2b-160 hash
   * @param to_hash - data to hash
   * @return hash
   */
  Blake2b160Hash blake2b_160(gsl::span<const uint8_t> to_hash);

  /**
   * @brief Get blake2b-256 hash
   * @param to_hash - data to hash
   * @return hash
   */
  Blake2b256Hash blake2b_256(gsl::span<const uint8_t> to_hash);

  Blake2b512Hash blake2b_512_from_file(std::ifstream &file_stream);

}  // namespace fc::crypto::blake2b

#endif  // CPP_FILECOIN_BLAKE2B160_HPP
