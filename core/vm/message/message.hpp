/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_HPP

#include <boost/variant.hpp>
#include <gsl/span>

#include "common/outcome.hpp"
#include "crypto/bls/bls_provider.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"

namespace fc::vm::message {

  /**
   * @brief Message error codes
   */
  enum class MessageError {
    INVALID_LENGTH = 1,
    SERIALIZATION_FAILURE,
    VERIFICATION_FAILURE
  };

  using crypto::signature::Signature;
  using primitives::BigInt;
  using primitives::address::Address;

  /**
   * @brief UnsignedMessage struct
   */
  struct UnsignedMessage {
    Address to;
    Address from;

    uint64_t nonce;

    BigInt value;

    BigInt gasPrice;
    BigInt gasLimit;

    uint64_t method;
    std::vector<uint8_t> params;
  };

  /**
   * @brief SignedMessage struct
   */
  struct SignedMessage {
    UnsignedMessage message;
    Signature signature;
  };

  constexpr uint64_t kMessageMaxSize = 32 * 1024;

};  // namespace fc::vm::message

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::vm::message, MessageError);

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_HPP
