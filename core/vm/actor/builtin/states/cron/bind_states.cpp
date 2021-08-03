/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/type_manager/universal_impl.hpp"

#include "vm/actor/builtin/states/cron/cron_actor_state.hpp"
#include "vm/actor/builtin/states/cron/v0/cron_actor_state.hpp"
#include "vm/actor/builtin/states/cron/v2/cron_actor_state.hpp"
#include "vm/actor/builtin/states/cron/v3/cron_actor_state.hpp"

UNIVERSAL_IMPL(states::CronActorState,
               v0::cron::CronActorState,
               v2::cron::CronActorState,
               v3::cron::CronActorState,
               v3::cron::CronActorState,
               v3::cron::CronActorState)
