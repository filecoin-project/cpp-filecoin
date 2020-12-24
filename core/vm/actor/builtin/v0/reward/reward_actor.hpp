/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/smoothing/alpha_beta_filter.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using NetworkTime = BigInt;

  struct State {
    StoragePower baseline_power, realized_power;
    SpaceTime sum_baseline, sum_realized;
    NetworkTime effective_time;
    TokenAmount simple_supply, baseline_supply, last_per_epoch_reward;
    ChainEpoch reward_epochs_paid{};
  };
  CBOR_TUPLE(State,
             baseline_power,
             realized_power,
             sum_baseline,
             sum_realized,
             effective_time,
             simple_supply,
             baseline_supply,
             last_per_epoch_reward,
             reward_epochs_paid)

  struct Constructor : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  struct AwardBlockReward : ActorMethodBase<2> {
    struct Params {
      Address miner;
      TokenAmount penalty;
      TokenAmount gas_reward;
      int64_t tickets;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AwardBlockReward::Params, miner, penalty, gas_reward, tickets)

  struct ThisEpochReward : ActorMethodBase<3> {
    struct Result {
      TokenAmount this_epoch_reward;
      FilterEstimate this_epoch_reward_smoothed;
      StoragePower this_epoch_baseline_power;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ThisEpochReward::Result,
             this_epoch_reward,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power)

  struct UpdateNetworkKPI : ActorMethodBase<4> {
    using Params = StoragePower;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::reward
