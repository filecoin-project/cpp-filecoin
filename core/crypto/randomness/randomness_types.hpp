/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/bytes.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::crypto::randomness {

  /// @brief randomness value type
  using Randomness = common::Hash256;

  constexpr size_t kRandomnessLength = 32;

  /// @brief domain separation tag enum
  enum class DomainSeparationTag : uint64_t {
    TicketProduction = 1,
    ElectionProofProduction,
    WinningPoStChallengeSeed,
    WindowedPoStChallengeSeed,
    SealRandomness,
    InteractiveSealChallengeSeed,
    WindowedPoStDeadlineAssignment,
    MarketDealCronSeed,
    PoStChainCommit,
  };

  inline Randomness drawRandomness(gsl::span<const uint8_t> base,
                                   DomainSeparationTag tag,
                                   primitives::ChainEpoch round,
                                   gsl::span<const uint8_t> entropy) {
    Bytes buffer;
    putUint64(buffer, static_cast<uint64_t>(tag));
    append(buffer, crypto::blake2b::blake2b_256(base));
    putUint64(buffer, round);
    append(buffer, entropy);
    return crypto::blake2b::blake2b_256(buffer);
  }
}  // namespace fc::crypto::randomness
