/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_POLICY_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_POLICY_HPP

#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::market {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::piece::PaddedPieceSize;

  template <typename T>
  struct Bounds {
    bool in(T value) const {
      return min <= value && value <= max;
    }

    T min, max;
  };

  inline Bounds<EpochDuration> dealDurationBounds(PaddedPieceSize) {
    return {0, 10000};
  }

  inline Bounds<TokenAmount> dealPricePerEpochBounds(PaddedPieceSize,
                                                     EpochDuration) {
    return {0, 1 << 20};
  }

  inline Bounds<TokenAmount> dealProviderCollateralBounds(PaddedPieceSize,
                                                          EpochDuration) {
    return {0, 1 << 20};
  }

  inline Bounds<TokenAmount> dealClientCollateralBounds(PaddedPieceSize,
                                                        EpochDuration) {
    return {0, 1 << 20};
  }

  inline TokenAmount collateralPenaltyForDealActivationMissed(
      TokenAmount provider_collateral) {
    return provider_collateral;
  }
}  // namespace fc::vm::actor::builtin::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_POLICY_HPP
