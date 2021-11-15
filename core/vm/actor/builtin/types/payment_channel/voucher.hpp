/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/fwd.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::types::payment_channel {
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
    Bytes params;

    inline bool operator==(const ModularVerificationParameter &rhs) const {
      return actor == rhs.actor && method == rhs.method && params == rhs.params;
    }
  };
  CBOR_TUPLE(ModularVerificationParameter, actor, method, params)

  struct SignedVoucher {
    Address channel;
    ChainEpoch time_lock_min{};
    ChainEpoch time_lock_max{};
    Bytes secret_preimage;
    boost::optional<ModularVerificationParameter> extra;
    LaneId lane{};
    uint64_t nonce{};
    TokenAmount amount;
    ChainEpoch min_close_height{};
    std::vector<Merge> merges{};
    boost::optional<Bytes> signature_bytes;

    inline bool operator==(const SignedVoucher &rhs) const {
      return channel == rhs.channel && time_lock_min == rhs.time_lock_min
             && time_lock_max == rhs.time_lock_max
             && secret_preimage == rhs.secret_preimage && extra == rhs.extra
             && lane == rhs.lane && nonce == rhs.nonce && amount == rhs.amount
             && min_close_height == rhs.min_close_height && merges == rhs.merges
             && signature_bytes == rhs.signature_bytes;
    }

    inline outcome::result<Bytes> signingBytes() const {
      auto copy = *this;
      copy.signature_bytes = boost::none;
      return codec::cbor::encode(copy);
    }
  };
  FC_OPERATOR_NOT_EQUAL(SignedVoucher)
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
    Bytes extra;
    Bytes proof;
  };
  CBOR_TUPLE(PaymentVerifyParams, extra, proof)
}  // namespace fc::vm::actor::builtin::types::payment_channel
