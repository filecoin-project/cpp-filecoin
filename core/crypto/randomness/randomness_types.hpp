/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace fc::crypto::randomness {

  /// @brief randomness value type
  using Randomness = common::Hash256;

  /// @brief Serialization value type
  using Serialization = common::Buffer;

  /// @brief domain separation tag enum
  enum class DomainSeparationTag : size_t {
    TicketDrawingDST = 1,
    TicketProductionDST = 2,
    PoStDST = 3,
    SealRandomness,
    InteractiveSealChallengeSeed,
  };

}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_HPP
