/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/v2/monies.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using types::miner::VestSpec;
  using version::NetworkVersion;

  class Monies : public v2::miner::Monies {
   private:
    const BigInt locked_reward_factor_num = BigInt(75);
    const BigInt locked_reward_factor_denom = BigInt(100);

    const ChainEpoch invalid_window_po_st_projection_period =
        ChainEpoch(continued_fault_projection_period + 2 * kEpochsInDay);

    const BigInt base_reward_for_disputed_window_po_st =
        BigInt(4) * kFilecoinPrecision;
    const BigInt base_penalty_for_disputed_window_po_st =
        BigInt(20) * kFilecoinPrecision;

   public:
    outcome::result<TokenAmount> pledgePenaltyForInvalidWindowPoSt(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_power_estimate,
        const StoragePower &sector_power) const override;

    outcome::result<std::pair<TokenAmount, VestSpec>> lockedRewardFromReward(
        const TokenAmount &reward,
        const NetworkVersion &default_version) const override;
  };
  CBOR_NON(Monies);

}  // namespace fc::vm::actor::builtin::v3::miner
