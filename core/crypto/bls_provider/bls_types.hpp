/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CRYPTO_BLS_PROVIDER_TYPES_HPP
#define CRYPTO_BLS_PROVIDER_TYPES_HPP

#include <array>

extern "C" {
#include "crypto/bls_provider/libbls_signatures.h"
}

#include "common/outcome.hpp"

namespace fc::crypto::bls {

  using PrivateKey = std::array<uint8_t, PRIVATE_KEY_BYTES>;
  using PublicKey = std::array<uint8_t, PUBLIC_KEY_BYTES>;
  using Signature = std::array<uint8_t, SIGNATURE_BYTES>;
  using Digest = std::array<uint8_t, DIGEST_BYTES>;

  struct KeyPair {
      PrivateKey private_key;
      PublicKey public_key;
  };

  enum class Errors {
      InternalError = 1,
      KeyPairGenerationFailed,
      SignatureGenerationFailed,
      SignatureVerificationFailed,
      InvalidPrivateKey,
      InvalidPublicKey
  };
};

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::bls, Errors);

#endif
