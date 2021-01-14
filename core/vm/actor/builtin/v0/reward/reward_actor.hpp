/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REWARD_ACTOR_HPP
#define CPP_FILECOIN_REWARD_ACTOR_HPP

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using primitives::ChainEpoch;
  using primitives::FilterEstimate;
  using primitives::SpaceTime;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using NetworkTime = BigInt;

  struct State {
    StoragePower cumsum_baseline, cumsum_realized;
    ChainEpoch effective_network_time{};
    StoragePower effective_baseline_power;
    TokenAmount this_epoch_reward;
    FilterEstimate this_epoch_reward_smoothed;
    StoragePower this_epoch_baseline_power;
    ChainEpoch epoch{};
    TokenAmount total_mined;
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
             total_mined)

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

  struct LastPerEpochReward : ActorMethodBase<3> {
    using Result = TokenAmount;
    ACTOR_METHOD_DECL();
  };

  struct UpdateNetworkKPI : ActorMethodBase<4> {
    using Params = StoragePower;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::reward

#endif  // CPP_FILECOIN_REWARD_ACTOR_HPP
