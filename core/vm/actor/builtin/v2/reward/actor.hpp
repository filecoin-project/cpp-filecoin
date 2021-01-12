/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using primitives::ChainEpoch;
  using primitives::FilterEstimate;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;

  struct State {
    SpaceTime cumsum_baseline, cumsum_realized;
    ChainEpoch effective_network_time{};
    StoragePower effective_baseline_power;
    TokenAmount this_epoch_reward;
    FilterEstimate this_epoch_reward_smoothed;
    StoragePower this_epoch_baseline_power;
    ChainEpoch epoch{};
    TokenAmount total_storage_power_reward, simple_total, baseline_total;
  };
  CBOR_TUPLE(State,
             cumsum_baseline,
             cumsum_realized,
             effective_network_time,
             effective_baseline_power,
             this_epoch_reward,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power,
             epoch,
             total_storage_power_reward,
             simple_total,
             baseline_total)
}  // namespace fc::vm::actor::builtin::v2::reward
