/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address.hpp"

#include "common/visitor.hpp"
#include "crypto/blake2/blake2b160.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives, AddressError, e) {
  using fc::primitives::AddressError;
  switch (e) {
    case (AddressError::UNKNOWN_PROTOCOL):
      return "Failed to create address: unknown address protocol";
    case (AddressError::INVALID_PAYLOAD):
      return "Failed to create address: invalid payload for the specified "
             "protocol";
    case (AddressError::UNKNOWN_NETWORK):
      return "Failed to create address: network must either be MAINNET or "
             "TESTNET";
    default:
      return "Failed to create address: unknown error";
  };
}

namespace fc::primitives {

  bool Address::isKeyType() const {
    return visit_in_place(
        data,
        [](uint64_t v) { return false; },
        [](const Secp256k1PublicKeyHash &v) { return true; },
        [](const ActorExecHash &v) { return false; },
        [](const BLSPublicKeyHash &v) { return true; });
  }

  Protocol Address::getProtocol() const {
    return visit_in_place(
        data,
        [](uint64_t v) { return Protocol::ID; },
        [](const Secp256k1PublicKeyHash &v) { return Protocol::SECP256K1; },
        [](const ActorExecHash &v) { return Protocol::ACTOR; },
        [](const BLSPublicKeyHash &v) { return Protocol::BLS; });
  }

  fc::outcome::result<bool> Address::verifySyntax(
      const gsl::span<uint8_t> &seed_data) const {
    return visit_in_place(
        data,
        [](uint64_t v) { return true; },
        [&seed_data](
            const Secp256k1PublicKeyHash &v) -> fc::outcome::result<bool> {
          // seed_data is a blake2b-160 hash of a public key
          OUTCOME_TRY(hash, fc::crypto::blake2b::blake2b_160(seed_data));
          return std::equal(hash.begin(), hash.end(), v.begin());
        },
        [&seed_data](const ActorExecHash &v) -> fc::outcome::result<bool> {
          OUTCOME_TRY(hash, fc::crypto::blake2b::blake2b_160(seed_data));
          return std::equal(hash.begin(), hash.end(), v.begin());
        },
        [&seed_data](const BLSPublicKeyHash &v) {
          // seed_data is a public key
          return std::equal(seed_data.begin(), seed_data.end(), v.begin());
        });
  }

  bool operator==(const Address &lhs, const Address &rhs) {
    return lhs.network == rhs.network && lhs.getProtocol() == rhs.getProtocol()
           && lhs.data == rhs.data;
  }

  bool operator<(const Address &lhs, const Address &rhs) {
    return lhs.network < rhs.network
           || (lhs.network == rhs.network
               && (lhs.getProtocol() < rhs.getProtocol()
                   || (lhs.getProtocol() == rhs.getProtocol()
                       && lhs.data < rhs.data)));
  }

};  // namespace fc::primitives
