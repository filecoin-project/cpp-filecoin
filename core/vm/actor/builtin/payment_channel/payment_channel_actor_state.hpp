/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP

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
  using primitives::UBigInt;
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
    UBigInt to_send{};
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
    UBigInt amount;
    uint64_t min_close_height{};
    std::vector<Merge> merges{};
    Signature signature;
  };

  /**
   * CBOR serialization of LaneState
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const LaneState &state) {
    return s << (s.list() << state.state << state.redeem << state.nonce);
  }

  /**
   * CBOR deserialization of LaneState
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, LaneState &state) {
    s.list() >> state.state >> state.redeem >> state.nonce;
    return s;
  }

  /**
   * CBOR serialization of PaymentChannelActorState
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const PaymentChannelActorState &state) {
    return s << (s.list() << state.from << state.to << state.to_send
                          << state.settling_at << state.min_settling_height
                          << state.lanes);
  }

  /**
   * CBOR deserialization of PaymentChannelActorState
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, PaymentChannelActorState &state) {
    s.list() >> state.from >> state.to >> state.to_send >> state.settling_at
        >> state.min_settling_height >> state.lanes;
    return s;
  }

  /**
   * CBOR serialization of ModularVerificationParameter
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const ModularVerificationParameter &param) {
    return s << (s.list() << param.actor << param.method << param.data);
  }

  /**
   * CBOR deserialization of ModularVerificationParameter
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, ModularVerificationParameter &param) {
    s.list() >> param.actor >> param.method >> param.data;
    return s;
  }

  /**
   * CBOR serialization of Merge
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Merge &merge) {
    return s << (s.list() << merge.lane << merge.nonce);
  }

  /**
   * CBOR deserialization of Merge
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Merge &merge) {
    s.list() >> merge.lane >> merge.nonce;
    return s;
  }

  /**
   * CBOR serialization of SignedVoucher
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const SignedVoucher &voucher) {
    return s << (s.list() << voucher.time_lock << voucher.secret_preimage
                          << voucher.extra << voucher.lane << voucher.nonce
                          << voucher.amount << voucher.min_close_height
                          << voucher.merges << voucher.signature);
  }

  /**
   * CBOR deserialization of SignedVoucher
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, SignedVoucher &voucher) {
    s.list() >> voucher.time_lock >> voucher.secret_preimage >> voucher.extra
        >> voucher.lane >> voucher.nonce >> voucher.amount
        >> voucher.min_close_height >> voucher.merges >> voucher.signature;
    return s;
  }

}  // namespace fc::vm::actor::builtin::payment_channel
#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_PAYMENT_CHANNEL_ACTOR_STATE_HPP
