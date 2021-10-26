/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/system/system_actor_state.hpp"
#include "vm/actor/builtin/states/system/v0/system_actor_state.hpp"
#include "vm/actor/builtin/states/system/v2/system_actor_state.hpp"
#include "vm/actor/builtin/states/system/v3/system_actor_state.hpp"

UNIVERSAL_IMPL(states::SystemActorState,
               v0::system::SystemActorState,
               v2::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState)
