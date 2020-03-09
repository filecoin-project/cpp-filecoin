/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_HPP

#include "filecoin/codec/cbor/cbor.hpp"
#include <gsl/span>

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/common/outcome.hpp"
#include "filecoin/crypto/signature/signature.hpp"
#include "filecoin/primitives/address/address.hpp"
#include "filecoin/primitives/address/address_codec.hpp"
#include "filecoin/primitives/big_int.hpp"
#include "filecoin/storage/ipld/ipld_block_common.hpp"
#include "filecoin/vm/actor/actor.hpp"

namespace fc::vm::message {

  /**
   * @brief Message error codes
   */
  enum class MessageError {
    INVALID_LENGTH = 1,
    SERIALIZATION_FAILURE,
    VERIFICATION_FAILURE
  };

  using actor::MethodNumber;
  using actor::MethodParams;
  using crypto::signature::Signature;
  using primitives::BigInt;
  using primitives::address::Address;
  using storage::ipld::IPLDBlockCommon;

  /**
   * @brief UnsignedMessage struct
   */
  struct UnsignedMessage
      : public IPLDBlockCommon<CID::Version::V1,
                               storage::ipld::HashType::blake2b_256,
                               storage::ipld::ContentType::DAG_CBOR> {
    UnsignedMessage() = default;

    UnsignedMessage(Address to,
                    Address from,
                    uint64_t nonce,
                    BigInt value,
                    BigInt gasPrice,
                    BigInt gasLimit,
                    MethodNumber method,
                    MethodParams params)
        : to{std::move(to)},
          from{std::move(from)},
          nonce{nonce},
          value{std::move(value)},
          gasPrice{std::move(gasPrice)},
          gasLimit{std::move(gasLimit)},
          method{method},
          params{std::move(params)} {}

    Address to;
    Address from;

    uint64_t nonce{};

    BigInt value{};

    BigInt gasPrice{};
    BigInt gasLimit{};

    MethodNumber method{};
    MethodParams params{};

    outcome::result<std::vector<uint8_t>> getBlockContent() const override {
      return codec::cbor::encode<UnsignedMessage>(*this);
    }

    /**
     * @brief Message equality operator
     */
    bool operator==(const UnsignedMessage &other) const;

    /**
     * @brief Message not equality operator
     */
    bool operator!=(const UnsignedMessage &other) const;

    BigInt requiredFunds() const;
  };

  CBOR_TUPLE(UnsignedMessage,
             to,
             from,
             nonce,
             value,
             gasPrice,
             gasLimit,
             method,
             params)

  /**
   * @brief SignedMessage struct
   */
  struct SignedMessage {
    UnsignedMessage message;
    Signature signature;
  };

  CBOR_TUPLE(SignedMessage, message, signature)

  constexpr uint64_t kMessageMaxSize = 32 * 1024;

};  // namespace fc::vm::message

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::vm::message, MessageError);

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_HPP
