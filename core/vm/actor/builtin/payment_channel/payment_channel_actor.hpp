/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::actor::builtin::payment_channel {

  using fc::vm::runtime::InvocationOutput;
  using fc::vm::runtime::Runtime;

  /**
   * Construct method parameters
   */
  struct ConstructParameteres {
    /** Voucher receiver */
    Address to;
  };

  /**
   * UpdateChannelState method parameters
   */
  struct UpdateChannelStateParameters {
    SignedVoucher signed_voucher;
    Buffer secret;
    Buffer proof;
  };

  class PaymentChannelActor {
   public:
    static ACTOR_METHOD(construct);

    static ACTOR_METHOD(updateChannelState);

    static ACTOR_METHOD(settle);

    static ACTOR_METHOD(collect);
  };

  CBOR_TUPLE(ConstructParameteres, to)

  CBOR_TUPLE(UpdateChannelStateParameters, signed_voucher, secret, proof)

}  // namespace fc::vm::actor::builtin::payment_channel

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_HPP
