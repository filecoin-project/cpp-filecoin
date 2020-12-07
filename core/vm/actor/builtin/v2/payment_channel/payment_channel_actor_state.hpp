/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_PAYMENT_CHANNEL_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_PAYMENT_CHANNEL_ACTOR_STATE_HPP

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::payment_channel {

  /**
   * Payment channel actor state v2 is identical to Payment channel actor state v0
   */

  using LaneState = v0::payment_channel::LaneState;
  using State = v0::payment_channel::State;
  using Merge = v0::payment_channel::Merge;
  using ModularVerificationParameter =
      v0::payment_channel::ModularVerificationParameter;
  using SignedVoucher = v0::payment_channel::SignedVoucher;

}  // namespace fc::vm::actor::builtin::v2::payment_channel

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V2_PAYMENT_CHANNEL_ACTOR_STATE_HPP
