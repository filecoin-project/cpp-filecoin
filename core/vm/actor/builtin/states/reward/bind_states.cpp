/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/type_manager/universal_impl.hpp"

#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/states/reward/v0/reward_actor_state.hpp"
#include "vm/actor/builtin/states/reward/v2/reward_actor_state.hpp"
// TODO reward state v3

UNIVERSAL_IMPL(states::RewardActorState,
               v0::reward::RewardActorState,
               v2::reward::RewardActorState,
               v2::reward::RewardActorState,
               v2::reward::RewardActorState,
               v2::reward::RewardActorState)
