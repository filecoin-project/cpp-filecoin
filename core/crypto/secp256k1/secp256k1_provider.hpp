/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include "common/outcome.hpp"
#include "crypto/secp256k1/secp256k1_types.hpp"

namespace fc::crypto::secp256k1 {

  /**
   * Interface is expanded with pubkey recovery from signature.
   * By default it uses following formats according to go-crypto:
   * - public key in uncompressed form
   * - signature in compact format
   *
   * @tparam KeyPairType - type of keypair
   * @tparam PublicKeyType - public key type (compressed or uncompressed)
   * @tparam SignatureType - signature format type (DER or compact)
   */
  template <typename KeyPairType,
            typename PublicKeyType,
            typename SignatureType>
  class Secp256k1Provider {
   public:
    virtual ~Secp256k1Provider() = default;

    /**
     * @brief Generate private and public keys
     * @return Secp256k1 key pair or error code
     */
    virtual outcome::result<KeyPairType> generate() const = 0;

    /**
     * @brief Generate public key from private key
     * @param key - private key for deriving public key
     * @return Derived public key or error code
     */
    virtual outcome::result<PublicKeyType> derive(
        const PrivateKey &key) const = 0;

    /**
     * @brief Create signature for a message
     * @param message - data to signing
     * @param key - private key for signing
     * @return Secp256k1 signature or error code
     */
    virtual outcome::result<SignatureType> sign(
        gsl::span<const uint8_t> message, const PrivateKey &key) const = 0;

    /**
     * @brief Verify signature for a message
     * @param message - signed data
     * @param signature - target for verifying
     * @param key - public key for signature verifying
     * @return Result of the verification or error code
     */
    virtual outcome::result<bool> verify(gsl::span<const uint8_t> message,
                                         const SignatureType &signature,
                                         const PublicKeyType &key) const = 0;

    /**
     * RecoverPubkey returns the the public key of the signer.
     * @param message - signed data
     * @param signature - target for verifying
     * @return Derived public key or error code
     */
    virtual outcome::result<PublicKeyType> recoverPublicKey(
        gsl::span<const uint8_t> message,
        const SignatureType &signature) const = 0;
  };

  using Secp256k1ProviderDefault =
      Secp256k1Provider<KeyPair, PublicKeyUncompressed, SignatureCompact>;

}  // namespace fc::crypto::secp256k1
