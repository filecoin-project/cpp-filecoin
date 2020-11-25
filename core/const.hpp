/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>

namespace fc {
  using BigInt = boost::multiprecision::cpp_int;

  constexpr auto kBaseFeeMaxChangeDenom{8};
  constexpr auto kBlockGasLimit{10000000000};
  constexpr auto kBlockGasTarget{kBlockGasLimit / 2};
  constexpr auto kBlockMessageLimit{10000};
  constexpr auto kBlocksPerEpoch{5};
  constexpr auto kConsensusMinerMinMiners{3};
  constexpr auto kEpochDurationSeconds{30};
  constexpr auto kGasLimitOverestimation{1.25};
  constexpr auto kMessageConfidence{5};
  constexpr auto kMinimumBaseFee{100};
  constexpr auto kBlockDelaySecs{kEpochDurationSeconds};
  constexpr auto kPackingEfficiencyDenom{5};
  constexpr auto kPackingEfficiencyNum{4};
  constexpr auto kPropagationDelaySecs{6};
  constexpr auto kUpgradeSmokeHeight{51000};
  constexpr auto kSecondsInHour {60 * 60};
  constexpr auto kSecondsInDay {24 * kSecondsInHour};
  constexpr auto kEpochsInHour {kSecondsInHour / kEpochDurationSeconds};
  constexpr auto kEpochsInDay {24 * kEpochsInHour};

  extern BigInt kConsensusMinerMinPower;
}  // namespace fc
