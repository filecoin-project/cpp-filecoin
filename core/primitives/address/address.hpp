/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP

#include <cstdint>

#include <spdlog/fmt/fmt.h>

#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"

namespace fc::primitives::address {
  using Sec256k1PublicKey = crypto::secp256k1::PublicKey;
  using BlsPublicKey = crypto::bls::PublicKey;

  /**
   * @brief Potential errors creating and handling Filecoin addresses
   */
  enum class AddressError {
    kUnknownProtocol = 1, /**< Unknown Address protocol/type */
    kInvalidPayload,      /**< Invalid data for a given protocol */
    kUnknownNetwork       /**< Unknown network: neither testnet nor mainnet */
  };

  /**
   * @brief Supported networks inside which addresses make sense
   */
  enum Network : uint8_t { MAINNET = 0x0, TESTNET = 0x1 };

  // TODO(turuslan): FIL-118 remove hardcoded TESTNET
  constexpr auto kDefaultNetwork = TESTNET;

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

    bool isId() const;

    bool isBls() const;

    /**
     * Checks if the address is a Secp256k1
     * @return true if it is a Secp256k1
     */
    bool isSecp256k1() const;

    /// id - number assigned to actors in a Filecoin Chain
    static Address makeFromId(uint64_t id);

    static Address makeSecp256k1(const Sec256k1PublicKey &public_key);

    static Address makeActorExec(gsl::span<const uint8_t> data);

    static Address makeBls(const BlsPublicKey &public_key);

    uint64_t getId() const;

    bool verifySyntax(gsl::span<const uint8_t> seed_data) const;

    Payload data;
  };

  /**
   * @brief Addresses equality operator
   */
  bool operator==(const Address &lhs, const Address &rhs);

  /**
   * @brief Addresses not equality operator
   */
  bool operator!=(const Address &lhs, const Address &rhs);

  /**
   * @brief Addresses "less than" operator
   */
  bool operator<(const Address &lhs, const Address &rhs);

  std::string encodeToString(const Address &address);
};  // namespace fc::primitives::address

template <>
struct fmt::formatter<fc::primitives::address::Address>
    : formatter<std::string_view> {
  template <typename C>
  auto format(const fc::primitives::address::Address &address, C &ctx) {
    auto str = fc::primitives::address::encodeToString(address);
    return formatter<std::string_view>::format(str, ctx);
  }
};

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::address, AddressError);

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_HPP
