/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/universal/universal_impl.hpp"

#include "vm/actor/builtin/states/payment_channel/payment_channel_actor_state.hpp"
#include "vm/actor/builtin/states/payment_channel/v0/payment_channel_actor_state.hpp"

UNIVERSAL_IMPL(states::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState,
               v0::payment_channel::PaymentChannelActorState)
