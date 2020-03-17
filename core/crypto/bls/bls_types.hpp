/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CRYPTO_BLS_PROVIDER_TYPES_HPP
#define CRYPTO_BLS_PROVIDER_TYPES_HPP

#include <array>

#include "common/outcome.hpp"

namespace fc::crypto::bls {
  using PrivateKey = std::array<uint8_t, 32>;
  using PublicKey = std::array<uint8_t, 48>;
  using Signature = std::array<uint8_t, 96>;
  using Digest = std::array<uint8_t, 96>;

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
    InvalidPublicKey,
    AggregateError,
  };
};  // namespace fc::crypto::bls

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::bls, Errors);

#endif
