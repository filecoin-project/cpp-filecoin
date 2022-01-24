/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/builtin/states/init/v0/init_actor_state.hpp"

UNIVERSAL_IMPL(states::InitActorState,
               v0::init::InitActorState,
               v0::init::InitActorState,
               v0::init::InitActorState,
               v0::init::InitActorState,
               v0::init::InitActorState,
               v0::init::InitActorState,
               v0::init::InitActorState)
