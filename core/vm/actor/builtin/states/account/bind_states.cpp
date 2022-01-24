/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/account/account_actor_state.hpp"
#include "vm/actor/builtin/states/account/v0/account_actor_state.hpp"

UNIVERSAL_IMPL(states::AccountActorState,
               v0::account::AccountActorState,
               v0::account::AccountActorState,
               v0::account::AccountActorState,
               v0::account::AccountActorState,
               v0::account::AccountActorState,
               v0::account::AccountActorState,
               v0::account::AccountActorState)
