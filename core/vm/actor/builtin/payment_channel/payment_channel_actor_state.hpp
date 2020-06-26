/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP

#include "crypto/signature/signature.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::payment_channel {
  using common::Buffer;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;

  using LaneId = uint64_t;

  struct LaneState {
    LaneId id{};
    /** Total amount for vouchers have been redeemed from the lane */
    TokenAmount redeem{};
    uint64_t nonce{};

    inline bool operator==(const LaneState &other) const {
      return id == other.id && redeem == other.redeem && nonce == other.nonce;
    }
  };
  CBOR_TUPLE(LaneState, id, redeem, nonce)

  struct State {
    inline auto findLane(LaneId lane_id) {
      return std::lower_bound(
          lanes.begin(), lanes.end(), lane_id, [](auto &lane, auto lane_id) {
            return lane.id < lane_id;
          });
    }

    Address from;
    Address to;
    /** Token amount to send on collect after voucher was redeemed */
    TokenAmount to_send{};
    ChainEpoch settling_at;
    ChainEpoch min_settling_height;
    std::vector<LaneState> lanes{};
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
    Buffer data;

    inline bool operator==(const ModularVerificationParameter &rhs) const {
      return actor == rhs.actor && method == rhs.method && data == rhs.data;
    }
  };
  CBOR_TUPLE(ModularVerificationParameter, actor, method, data)

  struct SignedVoucher {
    ChainEpoch time_lock_min{};
    ChainEpoch time_lock_max{};
    Buffer secret_preimage;
    boost::optional<ModularVerificationParameter> extra;
    LaneId lane{};
    uint64_t nonce{};
    TokenAmount amount;
    ChainEpoch min_close_height{};
    std::vector<Merge> merges{};
    boost::optional<Signature> signature;

    inline bool operator==(const SignedVoucher &rhs) const {
      return time_lock_min == rhs.time_lock_min
             && time_lock_max == rhs.time_lock_max
             && secret_preimage == rhs.secret_preimage && extra == rhs.extra
             && lane == rhs.lane && nonce == rhs.nonce && amount == rhs.amount
             && min_close_height == rhs.min_close_height && merges == rhs.merges
             && signature == rhs.signature;
    }
  };
  CBOR_TUPLE(SignedVoucher,
             time_lock_min,
             time_lock_max,
             secret_preimage,
             extra,
             lane,
             nonce,
             amount,
             min_close_height,
             merges,
             signature)

  struct PaymentVerifyParams {
    Buffer extra;
    Buffer proof;
  };
  CBOR_TUPLE(PaymentVerifyParams, extra, proof);
}  // namespace fc::vm::actor::builtin::payment_channel

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
