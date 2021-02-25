/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"
#include "vm/actor/builtin/states/reward_actor_state.hpp"
#include "vm/actor/builtin/v2/reward/policy.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using primitives::StoragePower;
  using primitives::TokenAmount;

  struct RewardActorState : states::RewardActorState {
    RewardActorState() : states::RewardActorState(ActorVersion::kVersion2) {}

    void initialize(const StoragePower &current_realized_power) override;
    TokenAmount simpleTotal() const override;
    TokenAmount baselineTotal() const override;
  };
  CBOR_TUPLE(RewardActorState,
             cumsum_baseline,
             cumsum_realized,
             effective_network_time,
             effective_baseline_power,
             this_epoch_reward,
             this_epoch_reward_smoothed,
             this_epoch_baseline_power,
             epoch,
             total_reward,
             simple_total,
             baseline_total)

}  // namespace fc::vm::actor::builtin::v2::reward
