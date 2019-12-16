/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CRYPTO_MURMUR_MURMUR_HPP
#define CPP_FILECOIN_CRYPTO_MURMUR_MURMUR_HPP

#include <vector>

#include <gsl/span>

namespace fc::crypto::murmur {
  using Hash = std::array<uint8_t, 8>;

  Hash hash(gsl::span<const uint8_t> input);
}  // namespace fc::crypto::murmur

#endif  // CPP_FILECOIN_CRYPTO_MURMUR_MURMUR_HPP
