/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_PROVIDER_IMPL_HPP

#include "crypto/secp256k1/secp256k1_provider.hpp"

namespace fc::crypto::secp256k1 {

  class Secp256k1ProviderImpl : public Secp256k1Provider {
   public:
    outcome::result<PublicKey> recoverPublicKey(
        gsl::span<const uint8_t> message,
        const Signature &signature) const override;
  };

}  // namespace fc::crypto::secp256k1

#endif  // CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_PROVIDER_IMPL_HPP
