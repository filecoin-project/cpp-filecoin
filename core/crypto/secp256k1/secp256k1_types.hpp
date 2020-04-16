/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_TYPES_HPP
#define CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_TYPES_HPP

#include <array>

namespace fc::crypto::secp256k1 {

  static const size_t kPrivateKeyLength = 32;
  static const size_t kPublicKeyCompressedLength = 33;
  static const size_t kPublicKeyUncompressedLength = 65;
  static const size_t kMessageHashLength = 32;
  static const size_t kSignatureLength = 65;

  /**
   * @brief Common types
   */
  using PrivateKey = std::array<uint8_t, kPrivateKeyLength>;
  using PublicKeyCompressed = std::array<uint8_t, kPublicKeyCompressedLength>;
  using PublicKeyUncompressed =
  std::array<uint8_t, kPublicKeyUncompressedLength>;
  /**
   * Compact ECDSA signature format with a 65-byte signature with the recovery
   * id at the end
   */
  using SignatureCompact = std::array<uint8_t, kSignatureLength>;

  /**
   * DER ECDSA signature format
   */
  using SignatureDer = std::vector<uint8_t>;

  /**
   * @struct Key pair
   */
  struct KeyPair {
    PrivateKey private_key;           /**< Secp256k1 private key */
    PublicKeyUncompressed public_key; /**< Secp256k1 public uncompressed key */

    /**
     * @brief Comparing keypairs
     * @param other - second keypair to compare
     * @return true, if keypairs are equal
     */
    bool operator==(const KeyPair &other) {
      return private_key == other.private_key && public_key == other.public_key;
    }

    /**
     * @brief Comparing keypairs
     * @param other - second keypair to compare
     * @return true, if keypairs aren't equal
     */
    bool operator!=(const KeyPair &other) {
      return !operator==(other);
    }
  };

}

#endif  // CPP_FILECOIN_CORE_CRYPTO_SECP256K1_SECP256K1_TYPES_HPP
