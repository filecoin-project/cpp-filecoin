/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  ACTOR_STATE_TO_CBOR_THIS(PaymentChannelActorState)
}  // namespace fc::vm::actor::builtin::v0::payment_channel
