/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "crypto/secp256k1/secp256k1_types.hpp"
#include "secp256k1.h"

namespace fc::crypto::secp256k1 {

  /**
   * Implemetation of Secp256k1 provider with
   * - public key in uncompressed form
   * - signature in compact form
   * - NO digest function
   */
  class Secp256k1ProviderImpl : public Secp256k1ProviderDefault {
   public:
    Secp256k1ProviderImpl();

    outcome::result<KeyPair> generate() const override;

    outcome::result<PublicKeyUncompressed> derive(
        const PrivateKey &key) const override;

    outcome::result<SignatureCompact> sign(
        gsl::span<const uint8_t> message, const PrivateKey &key) const override;

    outcome::result<bool> verify(
        gsl::span<const uint8_t> message,
        const SignatureCompact &signature,
        const PublicKeyUncompressed &key) const override;

    outcome::result<PublicKeyUncompressed> recoverPublicKey(
        gsl::span<const uint8_t> message,
        const SignatureCompact &signature) const override;

   private:
    std::unique_ptr<secp256k1_context, void (*)(secp256k1_context *)> context_;

    static outcome::result<void> checkSignature(
        const SignatureCompact &signature);
  };

}  // namespace fc::crypto::secp256k1
