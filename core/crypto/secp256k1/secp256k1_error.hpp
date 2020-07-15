/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_ERROR_HPP
#define CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::crypto::secp256k1 {

  enum class Secp256k1Error {
    kKeyGenerationFailed = 1,
    kSignatureParseError,
    kSignatureSerializationError,
    kCannotSignError,
    kPubkeyParseError,
    kPubkeySerializationError,
    kRecoverError,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::secp256k1, Secp256k1Error);

#endif  // CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_ERROR_HPP
