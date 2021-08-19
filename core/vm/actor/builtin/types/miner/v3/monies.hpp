/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "vm/actor/builtin/types/miner/v2/monies.hpp"

namespace fc::vm::actor::builtin::v3::miner  {
  using fc::common::smoothing::FilterEstimate;
  using fc::primitives::ChainEpoch;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::actor::builtin::types::miner::VestSpec;
  using fc::vm::version::NetworkVersion;

  class Monies : public v2::miner::Monies {
   private:
    BigInt locked_reward_factor_num =
        BigInt(75);
    BigInt locked_reward_factor_denom =
        BigInt(100);

    ChainEpoch invalid_window_po_st_projection_period = ChainEpoch(continued_fault_projection_period + 2 * kEpochsInDay);

    BigInt base_reward_for_disputed_window_po_st =
        BigInt(4) * kFilecoinPrecision;
    BigInt base_penalty_for_disputed_window_po_st =
        BigInt(20) * kFilecoinPrecision;

   public:
    outcome::result<TokenAmount> PledgePenaltyForInvalidWindowPoSt(
        const FilterEstimate &reward_estimate,
        const FilterEstimate &network_qa_power_estimate,
        const StoragePower &qa_sector_power) override;

    outcome::result<std::pair<TokenAmount, VestSpec *>>
    LockedRewardFromReward(const TokenAmount &reward) override;
  };
  CBOR_NON(Monies);

}  // namespace fc::vm::actor::builtin::v3::miner