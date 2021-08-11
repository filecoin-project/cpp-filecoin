/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <codec/cbor/cbor_codec.hpp>
#include <gsl/span>

#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::message {

  /**
   * @brief Message error codes
   */
  enum class MessageError {
    kInvalidLength = 1,
    kSerializationFailure,
    kVerificationFailure
  };

  using actor::MethodNumber;
  using actor::MethodParams;
  using crypto::signature::Signature;
  using primitives::GasAmount;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::Address;

  static constexpr int64_t kMessageVersion = 0;
  static const TokenAmount kDefaultGasPrice = 0;
  static constexpr GasAmount kDefaultGasLimit = 1000000;

  /**
   * @brief UnsignedMessage struct
   */
  struct UnsignedMessage {
    UnsignedMessage() = default;

    UnsignedMessage(Address to,
                    Address from,
                    Nonce nonce,
                    TokenAmount value,
                    TokenAmount gas_fee_cap,
                    GasAmount gas_limit,
                    MethodNumber method,
                    MethodParams params)
        : version{kMessageVersion},
          to{std::move(to)},
          from{std::move(from)},
          nonce{nonce},
          value{std::move(value)},
          gas_limit{std::move(gas_limit)},
          gas_fee_cap{std::move(gas_fee_cap)},
          gas_premium{},
          method{method},
          params{std::move(params)} {}

    int64_t version{kMessageVersion};

    Address to;
    Address from;

    Nonce nonce{};

    TokenAmount value{};

    GasAmount gas_limit{};
    TokenAmount gas_fee_cap;
    TokenAmount gas_premium;

    MethodNumber method{};
    MethodParams params{};

    /**
     * @brief Message equality operator
     */
    bool operator==(const UnsignedMessage &other) const;

    TokenAmount requiredFunds() const;

    CID getCid() const;

    size_t chainSize() const;

    static outcome::result<UnsignedMessage> decode(BytesIn cbor);
  };
  FC_OPERATOR_NOT_EQUAL(UnsignedMessage)

  CBOR_TUPLE(UnsignedMessage,
             version,
             to,
             from,
             nonce,
             value,
             gas_limit,
             gas_fee_cap,
             gas_premium,
             method,
             params)

  /**
   * @brief SignedMessage struct
   */
  struct SignedMessage {
    UnsignedMessage message;
    Signature signature;

    /**
     * Returnes CID of signed message
     * @return CID
     */
    CID getCid() const;

    size_t chainSize() const;
  };

  CBOR_TUPLE(SignedMessage, message, signature)

  void capGasFee(UnsignedMessage &msg, const TokenAmount &max);
}  // namespace fc::vm::message

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::vm::message, MessageError);
