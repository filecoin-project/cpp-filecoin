/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry/v0/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry/v2/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/states/verified_registry/v3/verified_registry_actor_state.hpp"

UNIVERSAL_IMPL(states::VerifiedRegistryActorState,
               v0::verified_registry::VerifiedRegistryActorState,
               v2::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState)
