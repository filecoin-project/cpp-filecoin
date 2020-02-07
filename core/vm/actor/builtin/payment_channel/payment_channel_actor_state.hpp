/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP

#include "primitives/big_int.hpp"

namespace fc::vm::actor::builtin::payment_channel {

  using primitives::BigInt;

  struct LaneState {
    int64_t state;
    BigInt redeem;
    int64_t nonce;
  };

  struct PaymentChannelActorState {

  };

}  // namespace fc::vm::actor::builtin::payment_channel
#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
