/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/message.hpp"

namespace fc::vm::message {
  using primitives::BigInt;

  bool UnsignedMessage::operator==(const UnsignedMessage &other) const {
    return to == other.to && from == other.from && nonce == other.nonce
           && value == other.value && gas_limit == other.gas_limit
           && gas_fee_cap == other.gas_fee_cap
           && gas_premium == other.gas_premium && method == other.method
           && params == other.params;
  }

  bool UnsignedMessage::operator!=(const UnsignedMessage &other) const {
    return !(*this == other);
  }

  BigInt UnsignedMessage::requiredFunds() const {
    return gas_limit * gas_fee_cap;
  }

  size_t UnsignedMessage::chainSize() const {
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
    return bytes.size();
  }

  CID SignedMessage::getCid() const {
    if (signature.isBls()) {
      OUTCOME_EXCEPT(data, codec::cbor::encode(message));
      OUTCOME_EXCEPT(res, getCidOf(data));
      return res;
    }
    OUTCOME_EXCEPT(data, codec::cbor::encode(*this));
    OUTCOME_EXCEPT(res, getCidOf(data));
    return res;
  }

  size_t SignedMessage::chainSize() const {
    if (signature.isBls()) {
      return message.chainSize();
    }
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
    return bytes.size();
  }
}  // namespace fc::vm::message

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::message, MessageError, e) {
  using fc::vm::message::MessageError;
  switch (e) {
    case (MessageError::kInvalidLength):
      return "MessageError: invalid message length";
    case (MessageError::kSerializationFailure):
      return "MessageError: message serialization failed";
    case (MessageError::kVerificationFailure):
      return "MessageError: signature verification failed";
    default:
      return "Unknown error";
  };
}
