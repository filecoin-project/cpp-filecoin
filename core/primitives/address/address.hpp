/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP

#include <cstdint>

#include <boost/variant.hpp>
#include "common/blob.hpp"

namespace fc::primitives::address {

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

    /** id - number assigned to actors in a Filecoin Chain */
    static Address makeFromId(uint64_t id);

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

};  // namespace fc::primitives::address

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::address, AddressError);

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP
