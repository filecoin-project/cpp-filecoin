/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::vm::actor::builtin::paych {
  using primitives::EpochDuration;
  using types::payment_channel::SignedVoucher;

  // These methods must be actual with the last version of actors

  enum class PaymentChannelActor : MethodNumber {
    kConstruct = 1,
    kUpdateChannelState,
    kSettle,
    kCollect,
  };

  struct Construct
      : ActorMethodBase<MethodNumber(PaymentChannelActor::kConstruct)> {
    struct Params {
      Address from;
      Address to;

      inline bool operator==(const Params &other) const {
        return from == other.from && to == other.to;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(Construct::Params, from, to)

  struct UpdateChannelState : ActorMethodBase<MethodNumber(
                                  PaymentChannelActor::kUpdateChannelState)> {
    struct Params {
      SignedVoucher signed_voucher;
      Bytes secret;

      inline bool operator==(const Params &other) const {
        return signed_voucher == other.signed_voucher && secret == other.secret;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(UpdateChannelState::Params, signed_voucher, secret)

  struct Settle : ActorMethodBase<MethodNumber(PaymentChannelActor::kSettle)> {
  };

  struct Collect
      : ActorMethodBase<MethodNumber(PaymentChannelActor::kCollect)> {};

}  // namespace fc::vm::actor::builtin::paych
