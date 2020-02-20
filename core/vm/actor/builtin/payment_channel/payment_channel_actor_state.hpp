/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::payment_channel {

  using common::Buffer;
  using crypto::signature::Signature;
  using fc::vm::actor::MethodNumber;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::address::Address;

  struct LaneState {
    int64_t state{};
    BigInt redeem{};
    int64_t nonce{};

    inline bool operator==(const LaneState &other) const {
      return state == other.state && redeem == other.redeem
             && nonce == other.nonce;
    }
  };

  struct PaymentChannelActorState {
    Address from;
    Address to;
    BigInt to_send{};
    ChainEpoch settling_at;
    ChainEpoch min_settling_height;
    std::vector<LaneState> lanes{};
  };

  struct Merge {
    uint64_t lane{};
    uint64_t nonce{};
  };

  /**
   * Modular Verification method
   */
  struct ModularVerificationParameter {
    Address actor;
    MethodNumber method;
    Buffer data;
  };

  struct SignedVoucher {
    uint64_t time_lock{};
    Buffer secret_preimage;
    ModularVerificationParameter extra;
    uint64_t lane{};
    uint64_t nonce{};
    BigInt amount;
    uint64_t min_close_height{};
    std::vector<Merge> merges{};
    Signature signature;
  };

  CBOR_TUPLE(LaneState, state, redeem, nonce)

  CBOR_TUPLE(PaymentChannelActorState,
             from,
             to,
             to_send,
             settling_at,
             min_settling_height,
             lanes)

  CBOR_TUPLE(ModularVerificationParameter, actor, method, data)

  CBOR_TUPLE(Merge, lane, nonce)

  CBOR_TUPLE(SignedVoucher,
             time_lock,
             secret_preimage,
             extra,
             lane,
             nonce,
             amount,
             min_close_height,
             merges,
             signature)

}  // namespace fc::vm::actor::builtin::payment_channel
#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
