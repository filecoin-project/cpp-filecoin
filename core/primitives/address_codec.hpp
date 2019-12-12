/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_CODEC_HPP

#include <libp2p/crypto/secp256k1_types.hpp>
#include <string>
#include <vector>

#include "common/outcome.hpp"
#include "crypto/bls_provider/bls_types.hpp"
#include "primitives/address.hpp"

namespace fc::primitives {

  /**
   * @brief Encodes an Address to an array of bytes
   */
  std::vector<uint8_t> encode(const Address &address);

  /**
   * @brief Decodes an Address from an array of bytes
   */
  outcome::result<Address> decode(const std::vector<uint8_t> &v);

  /**
   * @brief Encodes an Address to a string
   */
  std::string encodeToString(const Address &address);

  /**
   * @brief Decodes an Address from a string
   */
  outcome::result<Address> decodeFromString(const std::string &s);

  /**
   * @brief A helper function that calculates a checksum of an Address protocol
   * + payload
   */
  std::vector<uint8_t> checksum(const Address &address);

  /**
   * @brief Validates whether the Address' checksum matches the provided expect
   */
  bool validateChecksum(const Address &address,
                        const std::vector<uint8_t> &expect);

  /**
   * @brief create address form Secp256k1 public key
   * @param public_key - Secp256k1 public key
   * @return address created from secp256k1 public key
   */
  fc::outcome::result<Address> makeFromPublicKey(
      const Network network,
      const libp2p::crypto::secp256k1::PublicKey &public_key);

  /**
   * @brief create address form BLS public key
   * @param public_key - BLS public key
   * @return address created from BLS public key
   */
  fc::outcome::result<Address> makeFromPublicKey(
      const Network network, const crypto::bls::PublicKey &public_key);

};  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_CODEC_HPP
