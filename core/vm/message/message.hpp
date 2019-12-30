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
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"

namespace fc::vm::message {

  /**
   * @brief Message error codes
   */
  enum class MessageError {
    INVALID_LENGTH = 1,
    INVALID_KEY_LENGTH,
    INVALID_SIGNATURE_LENGTH,
    WRONG_SIGNATURE_TYPE
  };

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

    uint64_t method;
    std::vector<uint8_t> params;

    BigInt gasPrice;
    BigInt gasLimit;
  };

  /**
   * @brief Signature struct
   */
  struct Signature {
    struct Secp256k1Signature : public crypto::secp256k1::Signature {
      using vector::vector;
    };

    struct BlsSignature : public crypto::bls::Signature {
      using array::array;
    };

    enum Type : uint8_t { SECP256K1 = 0x1, BLS = 0x2 };

    Type type;
    boost::variant<Secp256k1Signature, BlsSignature> data;
  };

  /**
   * @brief SignedMessage struct
   */
  struct SignedMessage {
    UnsignedMessage message;
    Signature signature;
  };

  extern const uint64_t kMessageMaxSize;
  extern const uint64_t kSignatureMaxLength;
  extern const uint64_t kBlsSignatureLength;
  extern const uint64_t kBlsPrivateKeyLength;
  extern const uint64_t kBlsPublicKeyLength;

};  // namespace fc::vm::message

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::vm::message, MessageError);

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_HPP
