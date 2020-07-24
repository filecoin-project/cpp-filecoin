/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_HPP
#define CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::crypto::randomness {

  /// @brief randomness value type
  using Randomness = common::Hash256;

  /// @brief domain separation tag enum
  enum class DomainSeparationTag : uint64_t {
    TicketProduction = 1,
    ElectionProofProduction,
    WinningPoStChallengeSeed,
    WindowedPoStChallengeSeed,
    SealRandomness,
    InteractiveSealChallengeSeed,
    WindowedPoStDeadlineAssignment,
  };

  inline Randomness drawRandomness(gsl::span<const uint8_t> base,
                                   DomainSeparationTag tag,
                                   primitives::ChainEpoch round,
                                   gsl::span<const uint8_t> entropy) {
    common::Buffer buffer;
    buffer.putUint64(static_cast<uint64_t>(tag));
    buffer.put(crypto::blake2b::blake2b_256(base));
    buffer.putUint64(round);
    buffer.put(entropy);
    return crypto::blake2b::blake2b_256(buffer);
  }
}  // namespace fc::crypto::randomness

#endif  // CPP_FILECOIN_CORE_CRYPTO_RANDOMNESS_RANDOMNESS_HPP
