/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host/context/crypto/crypto_context_impl.hpp"

namespace fc::host::context::crypto {
  std::shared_ptr<CryptoContextImpl::CryptoProvider>
  CryptoContextImpl::getProvider() const {
    return crypto_provider_;
  }

  std::shared_ptr<CryptoContextImpl::SecureRandomGenerator>
  CryptoContextImpl::getSecureRandomGenerator() const {
    return random_generator_;
  }

  std::shared_ptr<CryptoContextImpl::HmacProvider>
  CryptoContextImpl::getHmacProvider() const {
    return hmac_provider_;
  }
}  // namespace fc::host::context::crypto
