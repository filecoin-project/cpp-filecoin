/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_SHA256_HPP
#define FILECOIN_SHA256_HPP

#include <string_view>

#include <gsl/span>
#include "common/blob.hpp"

namespace fc::crypto {
  /**
   * Take a SHA-256 hash from string
   * @param input to be hashed
   * @return hashed bytes
   */
  common::Hash256 sha256(std::string_view input);

  /**
   * Take a SHA-256 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  common::Hash256 sha256(gsl::span<const uint8_t> input);
}  // namespace kagome::crypto

#endif  // FILECOIN_SHA256_HPP
