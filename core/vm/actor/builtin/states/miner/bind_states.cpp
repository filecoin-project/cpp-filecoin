/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/states/miner/v0/miner_actor_state.hpp"
#include "vm/actor/builtin/states/miner/v2/miner_actor_state.hpp"
#include "vm/actor/builtin/states/miner/v3/miner_actor_state.hpp"

UNIVERSAL_IMPL(states::MinerActorState,
               v0::miner::MinerActorState,
               v2::miner::MinerActorState,
               v3::miner::MinerActorState,
               v3::miner::MinerActorState,
               v3::miner::MinerActorState)
