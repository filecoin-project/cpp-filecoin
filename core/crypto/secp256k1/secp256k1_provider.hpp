/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_CRYPTO_SECP256K1_PROVIDER_SECP256K1_PROVIDER_HPP
#define CPP_FILECOIN_TEST_CORE_CRYPTO_SECP256K1_PROVIDER_SECP256K1_PROVIDER_HPP

#include <gsl/span>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>
#include "common/outcome.hpp"

namespace fc::crypto::secp256k1 {

  using libp2p::crypto::secp256k1::KeyPair;
  using libp2p::crypto::secp256k1::kPrivateKeyLength;
  using libp2p::crypto::secp256k1::kPublicKeyLength;
  using libp2p::crypto::secp256k1::PrivateKey;
  using libp2p::crypto::secp256k1::PublicKey;
  using libp2p::crypto::secp256k1::Signature;

  /**
   * Interface is expanded with pubkey recovery from signature.
   */
  class Secp256k1Provider
      : public libp2p::crypto::secp256k1::Secp256k1Provider {
   public:
    /**
     * RecoverPubkey returns the the public key of the signer.
     * @param message - signed data
     * @param signature - target for verifying
     * @return Derived public key or error code
     */
    virtual outcome::result<PublicKey> recoverPublicKey(
        gsl::span<const uint8_t> message, const Signature &signature) const = 0;
  };

}  // namespace fc::crypto::secp256k1

#endif  // CPP_FILECOIN_TEST_CORE_CRYPTO_SECP256K1_PROVIDER_SECP256K1_PROVIDER_HPP
