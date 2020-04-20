/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_SECP256K1_IMPL_SECP_256_K_1_SHA_256_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_CRYPTO_SECP256K1_IMPL_SECP_256_K_1_SHA_256_PROVIDER_IMPL_HPP

#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"

namespace fc::crypto::secp256k1 {

  /**
   * Implementation of secp256k1 provider with sha256 digest function
   */
  class Secp256k1Sha256ProviderImpl : public Secp256k1ProviderImpl {
   public:
    outcome::result<SignatureCompact> sign(
        gsl::span<const uint8_t> message, const PrivateKey &key) const override;

    outcome::result<bool> verify(
        gsl::span<const uint8_t> message,
        const SignatureCompact &signature,
        const PublicKeyUncompressed &key) const override;

    outcome::result<PublicKeyUncompressed> recoverPublicKey(
        gsl::span<const uint8_t> message,
        const SignatureCompact &signature) const override;
  };

}  // namespace fc::crypto::secp256k1

#endif  // CPP_FILECOIN_CORE_CRYPTO_SECP256K1_IMPL_SECP_256_K_1_SHA_256_PROVIDER_IMPL_HPP
