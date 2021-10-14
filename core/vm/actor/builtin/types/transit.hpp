/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types {
  // Common result types of different actors and methods.
  // These common types are used as universal results for different versions of
  // actors
  // Note: should not be CBORed

  using common::smoothing::FilterEstimate;
  using primitives::DealWeight;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;

  // RewardActor::ThisEpochReward::Result
  struct EpochReward {
    TokenAmount this_epoch_reward;
    FilterEstimate this_epoch_reward_smoothed;
    StoragePower this_epoch_baseline_power;
  };

  // StoragePowerActor::CurrentTotalPower::Result
  struct TotalPower {
    StoragePower raw_byte_power;
    StoragePower quality_adj_power;
    TokenAmount pledge_collateral;
    FilterEstimate quality_adj_power_smoothed;
  };

  // MinerActor::ControlAddresses::Result
  struct Controls {
    Address owner;
    Address worker;
    std::vector<Address> control;
  };

  // MarketActor::VerifyDealsForActivation::Result
  struct DealWeights {
    DealWeight deal_weight;
    DealWeight verified_deal_weight;
    uint64_t deal_space{};
  };

}  // namespace fc::vm::actor::builtin::types
