/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "const.hpp"
#include "primitives/piece/piece.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/shared.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::actor::builtin::types::market {
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::piece::PaddedPieceSize;
  using types::market::DealProposal;
  using version::NetworkVersion;

  inline const TokenAmount kTotalFilecoin{2000000000
                                          * TokenAmount{"1000000000000000000"}};

  /// The number of blocks between payouts for deals
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern EpochDuration kDealUpdatesInterval;

  /**
   * ProvCollateralPercentSupplyNum is the numerator of the percentage of
   * normalized circulating supply that must be covered by provider collateral
   */
  inline const BigInt kProvCollateralPercentSupplyNumV0{5};
  inline const BigInt kProvCollateralPercentSupplyNumV1{1};

  /**
   * Denominator of the percentage of normalized circulating supply that must be
   * covered by provider collateral
   */
  inline const BigInt kProvCollateralPercentSupplyDenom{100};

  // DealMaxLabelSize is the maximum size of a deal label.
  inline const int kDealMaxLabelSize = 256;

  template <typename T>
  struct Bounds {
    bool in(T value) const {
      return min <= value && value <= max;
    }

    T min;
    T max;
  };

  inline Bounds<EpochDuration> dealDurationBounds(PaddedPieceSize) {
    return {static_cast<EpochDuration>(180 * fc::kEpochsInDay),
            static_cast<EpochDuration>(540 * fc::kEpochsInDay)};
  }

  inline Bounds<TokenAmount> dealPricePerEpochBounds(PaddedPieceSize,
                                                     EpochDuration) {
    return {0, kTotalFilecoin};
  }

  inline StoragePower dealQAPower(const PaddedPieceSize &deal_size,
                                  bool verified) {
    BigInt scaled_up_quality{0};
    if (verified) {
      scaled_up_quality =
          bigdiv(kVerifiedDealWeightMultiplier << kSectorQualityPrecision,
                 kQualityBaseMultiplier);
    } else {
      scaled_up_quality =
          bigdiv(kDealWeightMultiplier << kSectorQualityPrecision,
                 kQualityBaseMultiplier);
    }
    scaled_up_quality *= static_cast<uint64_t>(deal_size);
    return scaled_up_quality >> kSectorQualityPrecision;
  }

  inline Bounds<TokenAmount> dealProviderCollateralBounds(
      const PaddedPieceSize &piece_size,
      bool verified,
      const StoragePower &network_raw_power,
      const StoragePower &network_qa_power,
      const StoragePower &baseline_power,
      const TokenAmount &network_circulating_supply,
      NetworkVersion network_version) {
    BigInt lock_target_num =
        kProvCollateralPercentSupplyNumV0 * network_circulating_supply;
    BigInt power_share_num = dealQAPower(piece_size, verified);
    BigInt power_share_denom =
        std::max(std::max(network_qa_power, baseline_power), power_share_num);

    if (network_version >= NetworkVersion::kVersion1) {
      lock_target_num =
          kProvCollateralPercentSupplyNumV1 * network_circulating_supply;
      power_share_num = static_cast<uint64_t>(piece_size);
      power_share_denom = std::max(std::max(network_raw_power, baseline_power),
                                   power_share_num);
    }

    const BigInt min_collateral =
        bigdiv(lock_target_num * power_share_num,
               kProvCollateralPercentSupplyDenom * power_share_denom);
    return {min_collateral, kTotalFilecoin};
  }

  inline Bounds<TokenAmount> dealClientCollateralBounds(PaddedPieceSize,
                                                        EpochDuration) {
    return {0, kTotalFilecoin};
  }

  inline TokenAmount collateralPenaltyForDealActivationMissed(
      TokenAmount provider_collateral) {
    return provider_collateral;
  }

  inline DealWeight dealWeight(const DealProposal &deal) {
    const auto deal_duration = static_cast<int64_t>(deal.duration());
    const auto deal_size = static_cast<uint64_t>(deal.piece_size);
    return deal_duration * deal_size;
  }
}  // namespace fc::vm::actor::builtin::types::market
