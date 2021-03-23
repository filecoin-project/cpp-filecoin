/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/outcome.hpp"

namespace fc {
  using common::Blob;
  using BlsPublicKey = Blob<48>;
  using BlsSignature = Blob<96>;
}  // namespace fc

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
    kInternalError = 1,
    kKeyPairGenerationFailed,
    kSignatureGenerationFailed,
    kSignatureVerificationFailed,
    kInvalidPrivateKey,
    kInvalidPublicKey,
    kAggregateError,
  };
}  // namespace fc::crypto::bls

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::bls, Errors);
