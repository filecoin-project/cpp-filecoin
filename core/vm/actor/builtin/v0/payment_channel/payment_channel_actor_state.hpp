/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_PAYMENT_CHANNEL_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_PAYMENT_CHANNEL_ACTOR_STATE_HPP

#include "adt/array.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  using common::Buffer;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;

  using LaneId = uint64_t;

  struct LaneState {
    /** Total amount for vouchers have been redeemed from the lane */
    TokenAmount redeem{};
    uint64_t nonce{};

    inline bool operator==(const LaneState &other) const {
      return redeem == other.redeem && nonce == other.nonce;
    }
  };
  CBOR_TUPLE(LaneState, redeem, nonce)

  struct State {
    Address from;
    Address to;
    /** Token amount to send on collect after voucher was redeemed */
    TokenAmount to_send{};
    ChainEpoch settling_at{};
    ChainEpoch min_settling_height{};
    adt::Array<LaneState> lanes;
  };
  CBOR_TUPLE(State, from, to, to_send, settling_at, min_settling_height, lanes)

  struct Merge {
    LaneId lane{};
    uint64_t nonce{};

    inline bool operator==(const Merge &rhs) const {
      return lane == rhs.lane && nonce == rhs.nonce;
    }
  };
  CBOR_TUPLE(Merge, lane, nonce)

  /**
   * Modular Verification method
   */
  struct ModularVerificationParameter {
    Address actor;
    MethodNumber method;
    Buffer params;

    inline bool operator==(const ModularVerificationParameter &rhs) const {
      return actor == rhs.actor && method == rhs.method && params == rhs.params;
    }
  };
  CBOR_TUPLE(ModularVerificationParameter, actor, method, params)

  struct SignedVoucher {
    Address channel;
    ChainEpoch time_lock_min{};
    ChainEpoch time_lock_max{};
    Buffer secret_preimage;
    boost::optional<ModularVerificationParameter> extra;
    LaneId lane{};
    uint64_t nonce{};
    TokenAmount amount;
    ChainEpoch min_close_height{};
    std::vector<Merge> merges{};
    boost::optional<Buffer> signature_bytes;

    inline bool operator==(const SignedVoucher &rhs) const {
      return channel == rhs.channel && time_lock_min == rhs.time_lock_min
             && time_lock_max == rhs.time_lock_max
             && secret_preimage == rhs.secret_preimage && extra == rhs.extra
             && lane == rhs.lane && nonce == rhs.nonce && amount == rhs.amount
             && min_close_height == rhs.min_close_height && merges == rhs.merges
             && signature_bytes == rhs.signature_bytes;
    }
  };
  CBOR_TUPLE(SignedVoucher,
             channel,
             time_lock_min,
             time_lock_max,
             secret_preimage,
             extra,
             lane,
             nonce,
             amount,
             min_close_height,
             merges,
             signature_bytes)

  struct PaymentVerifyParams {
    Buffer extra;
    Buffer proof;
  };
  CBOR_TUPLE(PaymentVerifyParams, extra, proof);
}  // namespace fc::vm::actor::builtin::v0::payment_channel

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::payment_channel::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::payment_channel::State &state,
                     const Visitor &visit) {
      visit(state.lanes);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_PAYMENT_CHANNEL_ACTOR_STATE_HPP
