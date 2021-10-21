/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/states/market/v0/market_actor_state.hpp"
#include "vm/actor/builtin/states/market/v2/market_actor_state.hpp"
#include "vm/actor/builtin/states/market/v3/market_actor_state.hpp"

UNIVERSAL_IMPL(states::MarketActorState,
               v0::market::MarketActorState,
               v2::market::MarketActorState,
               v3::market::MarketActorState,
               v3::market::MarketActorState,
               v3::market::MarketActorState)
