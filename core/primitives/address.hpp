/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP

#include <boost/variant.hpp>
#include <cstdint>
#include <libp2p/crypto/secp256k1_types.hpp>

#include "common/blob.hpp"
#include "common/outcome.hpp"
#include "crypto/bls_provider/bls_types.hpp"

namespace fc::primitives {

  /**
   * @brief Potential errors creating and handling Filecoin addresses
   */
  enum class AddressError {
    UNKNOWN_PROTOCOL = 1, /**< Unknown Address protocol/type */
    INVALID_PAYLOAD,      /**< Invalid data for a given protocol */
    UNKNOWN_NETWORK       /**< Unknown network: neither testnet nor mainnet */
  };

  /**
   * @brief Supported networks inside which addresses make sense
   */
  enum Network : uint8_t { MAINNET = 0x0, TESTNET = 0x1 };

  /**
   * @brief Known Address protocols
   */
  enum Protocol : uint8_t { ID = 0x0, SECP256K1 = 0x1, ACTOR = 0x2, BLS = 0x3 };

  struct Secp256k1PublicKeyHash : public common::Blob<20> {
    using Blob::Blob;
  };

  struct ActorExecHash : public common::Blob<20> {
    using Blob::Blob;
  };

  struct BLSPublicKeyHash : public common::Blob<48> {
    using Blob::Blob;
  };

  using Payload = boost::variant<uint64_t,
                                 Secp256k1PublicKeyHash,
                                 ActorExecHash,
                                 BLSPublicKeyHash>;

  /**
   * @brief Address refers to an actor in the Filecoin state
   */
  struct Address {
    /**
     * @brief Returns the address protocol: ID, Secp256k1, ACTOR or BLS
     */
    Protocol getProtocol() const;

    /**
     * @brief Public API method as in
     * https://filecoin-project.github.io/specs/#systems__filecoin_vm__actor__address
     * @return true if the address represents a public key
     */
    bool isKeyType() const;

    /**
     * Verify if seed_data is a base for address. If address is:
     * 0 - id - is always valid
     * 1 - sec256k1 - check payload field contains the Blake2b 160 hash of the
     * public key
     * 2 - actor - check payload field is Blake2b 160 hash of the meaningful
     * data
     * 3 - bls - check payload is a BLS public key
     * @param seed_data - data to generate address
     * @return true if data is a base for address
     */
    fc::outcome::result<bool> verifySyntax(
        const gsl::span<uint8_t> &seed_data) const;

    /**
     * @brief create address form Secp256k1 public key
     * @param public_key - Secp256k1 public key
     * @return address created from secp256k1 public key
     */
    static fc::outcome::result<Address> makeFromSecp256k1PublicKey(
        Network network,
        const libp2p::crypto::secp256k1::PublicKey &public_key);

    /**
     * @brief create address form BLS public key
     * @param public_key - BLS public key
     * @return address created from BLS public key
     */
    static fc::outcome::result<Address> makeFromBlsPublicKey(
        Network network, const crypto::bls::PublicKey &public_key);

   public:
    Network network;
    Payload data;
  };

  /**
   * @brief Addresses equality operator
   */
  bool operator==(const Address &lhs, const Address &rhs);

  /**
   * @brief Addresses "less than" operator
   */
  bool operator<(const Address &lhs, const Address &rhs);

};  // namespace fc::primitives

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives, AddressError);

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP
