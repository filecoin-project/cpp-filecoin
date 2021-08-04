/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/all_states.hpp"
#include "vm/actor/builtin/types/type_manager/universal_impl.hpp"

UNIVERSAL_IMPL(states::SystemActorState,
               v0::system::SystemActorState,
               v2::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState,
               v3::system::SystemActorState)

UNIVERSAL_IMPL(states::VerifiedRegistryActorState,
               v0::verified_registry::VerifiedRegistryActorState,
               v2::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState,
               v3::verified_registry::VerifiedRegistryActorState)
