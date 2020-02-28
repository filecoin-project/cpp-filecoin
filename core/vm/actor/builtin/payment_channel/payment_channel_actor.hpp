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

  struct Construct : ActorMethodBase<1> {
    struct Params {
      /** Voucher receiver */
      Address to;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, to)

  struct UpdateChannelState : ActorMethodBase<2> {
    struct Params {
      SignedVoucher signed_voucher;
      Buffer secret;
      Buffer proof;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(UpdateChannelState::Params, signed_voucher, secret, proof)

  struct Settle : ActorMethodBase<3> {
    ACTOR_METHOD_DECL();
  };

  struct Collect : ActorMethodBase<4> {
    ACTOR_METHOD_DECL();
  };

}  // namespace fc::vm::actor::builtin::payment_channel

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_HPP
