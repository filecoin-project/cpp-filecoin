/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CRYPTO_BLS_PROVIDER_HPP
#define CRYPTO_BLS_PROVIDER_HPP

#include <gsl/span>

#include "crypto/bls_provider/bls_types.hpp"

namespace fc::crypto::bls {
  /**
   * @class BLS provider - C++ wrapper around Rust BLS implementation
   */
  class BlsProvider {
   public:
    virtual ~BlsProvider() = default;
    /**
     * @brief Generate BLS key pair
     * @return BLS private and public keys or error code
     */
    virtual outcome::result<KeyPair> generateKeyPair() const = 0;

    /**
     * @brief Generate BLS public key from private
     * @param key - private BLS key
     * @return public BLS key or error code
     */
    virtual outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &key) const = 0;

    /**
     * @brief Generate BLS signature
     * @param message - data to sign
     * @param key - BLS private key
     * @return BLS signature or error code
     */
    virtual outcome::result<Signature> sign(gsl::span<const uint8_t> message,
                                            const PrivateKey &key) const = 0;

    /**
     * @brief Verify BLS signature
     * @param message - signed data
     * @param signature - BLS signature for verifying
     * @param key - BLS public key
     * @return signature status or error code
     */
    virtual outcome::result<bool> verifySignature(
        gsl::span<const uint8_t> message,
        const Signature &signature,
        const PublicKey &key) const = 0;
  };
}  // namespace fc::crypto::bls

#endif
