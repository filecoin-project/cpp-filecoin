/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sha/sha256.hpp"

namespace fc::crypto::sha {

  libp2p::common::Hash256 sha256(gsl::span<const uint8_t> input) {
    return libp2p::crypto::sha256(input).value();
  }
}  // namespace fc::crypto::sha
