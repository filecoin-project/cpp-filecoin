/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/v0/storage_power_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/v2/storage_power_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/v3/storage_power_actor_state.hpp"

UNIVERSAL_IMPL(states::PowerActorState,
               v0::storage_power::PowerActorState,
               v2::storage_power::PowerActorState,
               v3::storage_power::PowerActorState,
               v3::storage_power::PowerActorState,
               v3::storage_power::PowerActorState,
               v3::storage_power::PowerActorState)
