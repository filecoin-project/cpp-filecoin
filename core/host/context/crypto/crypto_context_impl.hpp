/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FUHON_CORE_HOST_CONTEXT_CRYPTO_IMPL_HPP
#define CPP_FUHON_CORE_HOST_CONTEXT_CRYPTO_IMPL_HPP

#include <memory>

#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>
#include <libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/crypto/rsa_provider/rsa_provider_impl.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>
#include "common/outcome.hpp"
#include "host/context/crypto_context.hpp"

namespace fc::host::context::crypto {
  class CryptoContextImpl : public CryptoContext {
   protected:
    using CryptoProviderImpl = libp2p::crypto::CryptoProviderImpl;
    using BoostRandomGenerator = libp2p::crypto::random::BoostRandomGenerator;
    using HmacProviderImpl = libp2p::crypto::hmac::HmacProviderImpl;
    using EcdsaProviderImpl = libp2p::crypto::ecdsa::EcdsaProviderImpl;
    using Ed25519ProviderImpl = libp2p::crypto::ed25519::Ed25519ProviderImpl;
    using RsaProviderImpl = libp2p::crypto::rsa::RsaProviderImpl;
    using Secp256k1ProviderImpl =
        libp2p::crypto::secp256k1::Secp256k1ProviderImpl;

   public:
    CryptoContextImpl()
        : random_generator_{std::make_shared<BoostRandomGenerator>()},
          hmac_provider_{std::make_shared<HmacProviderImpl>()},
          ed25519_provider_{std::make_shared<Ed25519ProviderImpl>()},
          rsa_provider_{std::make_shared<RsaProviderImpl>()},
          ecdsa_provider_{std::make_shared<EcdsaProviderImpl>()},
          secp256k1_provider_{std::make_shared<Secp256k1ProviderImpl>()},
          crypto_provider_{
              std::make_shared<CryptoProviderImpl>(random_generator_,
                                                   ed25519_provider_,
                                                   rsa_provider_,
                                                   ecdsa_provider_,
                                                   secp256k1_provider_,
                                                   hmac_provider_)} {}

    std::shared_ptr<CryptoProvider> getProvider() const override;

    std::shared_ptr<SecureRandomGenerator> getSecureRandomGenerator()
        const override;

    std::shared_ptr<HmacProvider> getHmacProvider() const override;

   private:
    std::shared_ptr<BoostRandomGenerator> random_generator_;
    std::shared_ptr<HmacProviderImpl> hmac_provider_;
    std::shared_ptr<Ed25519ProviderImpl> ed25519_provider_;
    std::shared_ptr<RsaProviderImpl> rsa_provider_;
    std::shared_ptr<EcdsaProviderImpl> ecdsa_provider_;
    std::shared_ptr<Secp256k1ProviderImpl> secp256k1_provider_;
    std::shared_ptr<CryptoProviderImpl> crypto_provider_;
  };
}  // namespace fc::host::context::crypto

#endif
