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

    inline bool operator!=(const LaneState &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(LaneState, redeem, nonce)

  struct Merge {
    LaneId lane{};
    uint64_t nonce{};

    inline bool operator==(const Merge &other) const {
      return lane == other.lane && nonce == other.nonce;
    }

    inline bool operator!=(const Merge &other) const {
      return !(*this == other);
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

    inline bool operator==(const ModularVerificationParameter &other) const {
      return actor == other.actor && method == other.method
             && params == other.params;
    }

    inline bool operator!=(const ModularVerificationParameter &other) const {
      return !(*this == other);
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

    inline bool operator==(const SignedVoucher &other) const {
      return channel == other.channel && time_lock_min == other.time_lock_min
             && time_lock_max == other.time_lock_max
             && secret_preimage == other.secret_preimage && extra == other.extra
             && lane == other.lane && nonce == other.nonce
             && amount == other.amount
             && min_close_height == other.min_close_height
             && merges == other.merges
             && signature_bytes == other.signature_bytes;
    }

    inline bool operator!=(const SignedVoucher &other) const {
      return !(*this == other);
    }

    inline outcome::result<Bytes> signingBytes() const {
      auto copy = *this;
      copy.signature_bytes = boost::none;
      return codec::cbor::encode(copy);
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
    Bytes extra;
    Bytes proof;

    inline bool operator==(const PaymentVerifyParams &other) const {
      return extra == other.extra && proof == other.proof;
    }

    inline bool operator!=(const PaymentVerifyParams &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(PaymentVerifyParams, extra, proof)
}  // namespace fc::vm::actor::builtin::types::payment_channel
