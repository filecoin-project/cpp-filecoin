/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/type_manager/universal_impl.hpp"

#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/builtin/states/init/v0/init_actor_state.hpp"
#include "vm/actor/builtin/states/init/v2/init_actor_state.hpp"
#include "vm/actor/builtin/states/init/v3/init_actor_state.hpp"

UNIVERSAL_IMPL(states::InitActorState,
               v0::init::InitActorState,
               v2::init::InitActorState,
               v3::init::InitActorState,
               v3::init::InitActorState,
               v3::init::InitActorState)
