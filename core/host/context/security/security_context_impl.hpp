/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FUHON_CORE_HOST_CONTEXT_SECURITY_IMPL_HPP
#define CPP_FUHON_CORE_HOST_CONTEXT_SECURITY_IMPL_HPP

#include <libp2p/crypto/key.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/peer/impl/identity_manager_impl.hpp>
#include <libp2p/security/secio.hpp>
#include <libp2p/security/secio/exchange_message_marshaller_impl.hpp>
#include <libp2p/security/secio/propose_message_marshaller_impl.hpp>
#include "host/context/crypto_context.hpp"
#include "host/context/security_context.hpp"

namespace fc::host::context::security {
  class SecurityContextImpl : public SecurityContext {
   protected:
    using CryptoContext = crypto::CryptoContext;
    using KeyValidatorImpl = libp2p::crypto::validator::KeyValidatorImpl;
    using KeyMarshallerImpl = libp2p::crypto::marshaller::KeyMarshallerImpl;
    using IdentityManagerImpl = libp2p::peer::IdentityManagerImpl;
    using KeyPair = libp2p::crypto::KeyPair;
    using SecureInputOutputAdaptor = libp2p::security::Secio;
    using ProposeMessageMarshallerImpl =
        libp2p::security::secio::ProposeMessageMarshallerImpl;
    using ExchangeMessageMarshallerImpl =
        libp2p::security::secio::ExchangeMessageMarshallerImpl;

   public:
    SecurityContextImpl(const std::shared_ptr<CryptoContext> &crypto_context,
                        const KeyPair &key_pair)
        : key_validator_{std::make_shared<KeyValidatorImpl>(
            crypto_context->getProvider())},
          key_marshaller_{std::make_shared<KeyMarshallerImpl>(key_validator_)},
          identity_manager_{
              std::make_shared<IdentityManagerImpl>(key_pair, key_marshaller_)},
          secio_adaptor_{std::make_shared<SecureInputOutputAdaptor>(
              crypto_context->getSecureRandomGenerator(),
              crypto_context->getProvider(),
              std::make_shared<ProposeMessageMarshallerImpl>(),
              std::make_shared<ExchangeMessageMarshallerImpl>(),
              identity_manager_,
              key_marshaller_,
              crypto_context->getHmacProvider())},
          security_adaptors_{secio_adaptor_} {}

    std::shared_ptr<IdentityManager> getIdentityManager() const override;

    const std::vector<SecurityAdaptorSPtr> &getSecurityAdaptors()
        const override;

   private:
    std::shared_ptr<KeyValidatorImpl> key_validator_;
    std::shared_ptr<KeyMarshallerImpl> key_marshaller_;
    std::shared_ptr<IdentityManagerImpl> identity_manager_;
    std::shared_ptr<SecureInputOutputAdaptor> secio_adaptor_;
    std::vector<SecurityAdaptorSPtr> security_adaptors_;
  };
}  // namespace fc::host::context::security

#endif
