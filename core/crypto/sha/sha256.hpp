/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/sha/sha256.hpp>

namespace fc::crypto::sha {
  using Hash256 = std::array<uint8_t, 32u>;

  Hash256 sha256(gsl::span<const uint8_t> input);
}  // namespace fc::crypto::sha
