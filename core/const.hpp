/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/types.hpp"

namespace fc {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::StoragePower;
  using primitives::TokenAmount;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern bool kFakeWinningPost;
  constexpr std::string_view kFakeWinningPostStr{"valid proof"};

  constexpr int64_t kSecondsInHour{60 * 60};
  constexpr int64_t kSecondsInDay{24 * kSecondsInHour};

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern size_t kEpochDurationSeconds;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern size_t kEpochsInHour;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern size_t kEpochsInDay;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern size_t kEpochsInYear;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern size_t kPropagationDelaySecs;

  constexpr auto kBaseFeeMaxChangeDenom{8};
  constexpr auto kBlockGasLimit{10000000000};
  constexpr auto kBlockGasTarget{kBlockGasLimit / 2};
  constexpr auto kBlockMessageLimit{10000};
  constexpr auto kBlocksPerEpoch{5};
  constexpr auto kConsensusMinerMinMiners{3};
  constexpr uint64_t kFilReserve{300000000};
  constexpr uint64_t kFilecoinPrecision{1000000000000000000};
  constexpr auto kGasLimitOverestimation{1.25};
  constexpr EpochDuration kMessageConfidence{5};
  const TokenAmount kMinimumBaseFee{100};
  constexpr auto kPackingEfficiencyDenom{5};
  constexpr auto kPackingEfficiencyNum{4};

  // ******************
  // Network versions
  // ******************
  /**
   * Network version heights. The last height before network upgrade.
   */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeBreezeHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeSmokeHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeIgnitionHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeRefuelHeight;
  /** Actors V2 update */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeAssemblyHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeTapeHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeLiftoffHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeKumquatHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeCalicoHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradePersianHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeOrangeHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeClausHeight;
  /** Actors V3 upgrade */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeTrustHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeNorwegianHeight;
  /** Actors V4 upgrade */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeTurboHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeHyperdriveHeight;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeChocolateHeight;
  /** Update introduces Snap deals */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern ChainEpoch kUpgradeOhSnapHeight;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern EpochDuration kBreezeGasTampingDuration;

  constexpr uint64_t kDefaultStorageWeight{10};

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern EpochDuration kInteractivePoRepConfidence;

  /**
   * Sets parameters for test network with 2k seal proof type
   * Identical to Lotus 2k build
   */
  void setParams2K();

  /**
   * Sets parameters for test network with high upgrade heights
   * May be useful when want to avoid network upgrades
   */
  void setParamsNoUpgrades();

  /**
   * Sets upgrades heights and parameters for Interopnet.
   * Identical to Lotus `build interopnet`
   */
  void setParamsInteropnet();
}  // namespace fc
