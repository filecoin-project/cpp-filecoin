/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_POLICY_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_POLICY_HPP

#include "primitives/piece/piece.hpp"
#include "vm/actor/builtin/v0/miner/policy.hpp"
#include "vm/actor/builtin/v0/shared/shared.hpp"
#include "vm/version.hpp"

namespace fc::vm::actor::builtin::v0::market {
    using miner::kEpochsInDay;
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::piece::PaddedPieceSize;
  using version::NetworkVersion;

  inline const TokenAmount kTotalFilecoin{2000000000
                                          * TokenAmount{"1000000000000000000"}};

  /// The number of blocks between payouts for deals
  constexpr EpochDuration kDealUpdatesInterval{kEpochsInDay};

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

  template <typename T>
  struct Bounds {
    bool in(T value) const {
      return min <= value && value <= max;
    }

    T min, max;
  };

  inline Bounds<EpochDuration> dealDurationBounds(PaddedPieceSize) {
    return {0, miner::kEpochsInYear};
  }

  inline Bounds<TokenAmount> dealPricePerEpochBounds(PaddedPieceSize,
                                                     EpochDuration) {
    return {0, kTotalFilecoin};
  }

  inline StoragePower dealQAPower(const PaddedPieceSize &piece_size,
                                  bool verified) {
    BigInt scaled_up_quality{0};
    if (verified) {
      scaled_up_quality =
          (kVerifiedDealWeightMultiplier << kSectorQualityPrecision)
          / kQualityBaseMultiplier;
    } else {
      scaled_up_quality = (kDealWeightMultiplier << kSectorQualityPrecision)
                          / kQualityBaseMultiplier;
    }
    scaled_up_quality *= uint64_t(piece_size);
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
      power_share_num = uint64_t(piece_size);
      power_share_denom = std::max(std::max(network_raw_power, baseline_power),
                                   power_share_num);
    }

    BigInt min_collateral =
        (lock_target_num * power_share_num)
        / (kProvCollateralPercentSupplyDenom * power_share_denom);
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
}  // namespace fc::vm::actor::builtin::v0::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_POLICY_HPP
