/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/program_options.hpp>

#include "common/outcome.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/types.hpp"

namespace fc {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::StoragePower;

  constexpr auto kSecondsInHour{60 * 60};
  constexpr auto kSecondsInDay{24 * kSecondsInHour};

  static size_t kEpochDurationSeconds{30};
  static size_t kEpochsInHour{kSecondsInHour / kEpochDurationSeconds};
  static size_t kEpochsInDay{24 * kEpochsInHour};
  static size_t kEpochsInYear{365 * kEpochsInDay};

  static size_t kPropagationDelaySecs{6};
  constexpr auto kBaseFeeMaxChangeDenom{8};
  constexpr auto kBlockGasLimit{10000000000};
  constexpr auto kBlockGasTarget{kBlockGasLimit / 2};
  constexpr auto kBlockMessageLimit{10000};
  constexpr auto kBlocksPerEpoch{5};
  constexpr auto kConsensusMinerMinMiners{3};
  constexpr uint64_t kFilReserve{300000000};
  constexpr uint64_t kFilecoinPrecision{1000000000000000000};
  constexpr auto kGasLimitOverestimation{1.25};
  constexpr auto kMessageConfidence{5};
  constexpr auto kMinimumBaseFee{100};
  constexpr auto kPackingEfficiencyDenom{5};
  constexpr auto kPackingEfficiencyNum{4};

  // ******************
  // Network versions
  // ******************
  /**
   * Network version heights. The last height before network upgrade.
   */
  static ChainEpoch kUpgradeBreezeHeight = 41280;
  static ChainEpoch kUpgradeSmokeHeight = 51000;
  static ChainEpoch kUpgradeIgnitionHeight = 94000;
  static ChainEpoch kUpgradeRefuelHeight = 130800;
  static ChainEpoch kUpgradeActorsV2Height = 138720;
  static ChainEpoch kUpgradeTapeHeight = 140760;
  static ChainEpoch kUpgradeLiftoffHeight = 148888;
  static ChainEpoch kUpgradeKumquatHeight = 170000;
  static ChainEpoch kUpgradeCalicoHeight = 265200;
  static ChainEpoch kUpgradePersianHeight = 272400;
  static ChainEpoch kUpgradeOrangeHeight = 336458;
  static ChainEpoch kUpgradeClausHeight = 343200;
  static ChainEpoch kUpgradeActorsV3Height = 550321;

  constexpr uint64_t kMinerApiVersion{0};
  constexpr uint64_t kDefaultStorageWeight{10};

  static EpochDuration kInteractivePoRepConfidence = 6;

  /**
   * Sets up program option description for 'profile' and initialize parameters
   * according the profile.
   *
   * @return profile program option description
   */
  boost::program_options::options_description configProfile();
}  // namespace fc
