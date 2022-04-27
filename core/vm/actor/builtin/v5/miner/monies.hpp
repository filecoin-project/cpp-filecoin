/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"

// TODO (m.tagirov) move to the proper directory
namespace fc::vm::actor::builtin::v5::miner {
  using common::math::kPrecision128;
  using common::smoothing::FilterEstimate;
  using primitives::TokenAmount;

  inline TokenAmount expectedRewardForPower(const FilterEstimate &reward,
                                            const FilterEstimate &network,
                                            const StoragePower &sector,
                                            uint64_t projection) {
    if (!estimate(network)) {
      return estimate(reward);
    }
    return std::max<TokenAmount>(
        0,
        (sector * extrapolatedCumSumOfRatio(projection, 0, reward, network))
            >> kPrecision128);
  }

  inline TokenAmount preCommitDepositForPower(const FilterEstimate &reward,
                                              const FilterEstimate &network,
                                              const StoragePower &sector) {
    return std::max<TokenAmount>(
        1, expectedRewardForPower(reward, network, sector, 20 * kEpochsInDay));
  }

  inline TokenAmount initialPledgeForPowerBase(const FilterEstimate &reward,
                                               const FilterEstimate &network,
                                               const StoragePower &sector) {
    return expectedRewardForPower(reward, network, sector, 20 * kEpochsInDay);
  }

  inline TokenAmount initialPledgeForPower(const TokenAmount &circ,
                                           const FilterEstimate &reward,
                                           const FilterEstimate &network,
                                           const StoragePower &sector,
                                           const StoragePower &baseline) {
    const auto base{std::max<TokenAmount>(
        1, initialPledgeForPowerBase(reward, network, sector))};
    const auto add{
        bigdiv(3 * sector * circ,
               10 * std::max(std::max(estimate(network), baseline), sector))};
    static const auto max{bigdiv(1000000000000000000, uint64_t{32} << 30)};
    return std::min<TokenAmount>(base + add, max * sector);
  }
}  // namespace fc::vm::actor::builtin::v5::miner
