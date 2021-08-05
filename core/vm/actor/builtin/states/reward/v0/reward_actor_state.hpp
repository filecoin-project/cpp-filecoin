/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using primitives::StoragePower;
  using primitives::TokenAmount;

  struct RewardActorState : states::RewardActorState {
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
             total_reward)

}  // namespace fc::vm::actor::builtin::v0::reward
