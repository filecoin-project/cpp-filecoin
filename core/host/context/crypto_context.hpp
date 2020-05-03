/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FUHON_CORE_HOST_CONTEXT_CRYPTO_HPP
#define CPP_FUHON_CORE_HOST_CONTEXT_CRYPTO_HPP

#include <memory>

#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/hmac_provider.hpp>
#include <libp2p/crypto/random_generator.hpp>

namespace fc::host::context::crypto {
  class CryptoContext {
   protected:
    using CryptoProvider = libp2p::crypto::CryptoProvider;
    using SecureRandomGenerator = libp2p::crypto::random::CSPRNG;
    using HmacProvider = libp2p::crypto::hmac::HmacProvider;

   public:
    virtual ~CryptoContext() = default;

    virtual std::shared_ptr<CryptoProvider> getProvider() const = 0;

    virtual std::shared_ptr<SecureRandomGenerator> getSecureRandomGenerator()
        const = 0;

    virtual std::shared_ptr<HmacProvider> getHmacProvider() const = 0;
  };
}  // namespace fc::host::context::crypto
#endif
