/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "common/smoothing/alpha_beta_filter.hpp"

namespace fc::vm::actor::builtin {
  using common::smoothing::FilterEstimate;
  using primitives::StoragePower;

  // These methods must be actual with the last version of actors

  enum class RewardActor : MethodNumber {
    kConstruct = 1,
    kAwardBlockReward,
    kThisEpochReward,
    kUpdateNetworkKPI,
  }

  struct Constructor : ActorMethodBase<RewardActor::kConstruct> {
    using Params = StoragePower;
  };

  struct AwardBlockReward : ActorMethodBase<RewardActor::kAwardBlockReward> {
    struct Params {
      Address miner;
      TokenAmount penalty;
      TokenAmount gas_reward;
      int64_t win_count{};

      inline bool operator==(const Params &other) const {
        return miner == other.miner && penalty == other.penalty
               && gas_reward == other.gas_reward
               && win_count == other.win_count;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(AwardBlockReward::Params, miner, penalty, gas_reward, win_count)

  struct ThisEpochReward : ActorMethodBase<RewardActor::kThisEpochReward> {
    struct Result {
      FilterEstimate this_epoch_reward_smoothed;
      StoragePower this_epoch_baseline_power;

      inline bool operator==(const Result &other) const {
        return this_epoch_reward_smoothed == other.this_epoch_reward_smoothed
               && this_epoch_baseline_power == other.this_epoch_baseline_power;
      }

      inline bool operator!=(const Result &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(ThisEpochReward::Result,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power)

  struct UpdateNetworkKPI : ActorMethodBase<RewardActor::kUpdateNetworkKPI> {
    using Params = StoragePower;
  };

}  // namespace fc::vm::actor::builtin
