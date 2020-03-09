/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/address/address.hpp"

#include "filecoin/common/visitor.hpp"
#include "filecoin/crypto/blake2/blake2b160.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::address, AddressError, e) {
  using fc::primitives::address::AddressError;
  switch (e) {
    case (AddressError::UNKNOWN_PROTOCOL):
      return "Failed to create address: unknown address protocol";
    case (AddressError::INVALID_PAYLOAD):
      return "Failed to create address: invalid payload for the specified "
             "protocol";
    case (AddressError::UNKNOWN_NETWORK):
      return "Failed to create address: network must either be MAINNET or "
             "TESTNET";
  }
  return "Failed to create address: unknown error";
}

namespace fc::primitives::address {
  using crypto::blake2b::blake2b_160;

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

  Address Address::makeFromId(uint64_t id, Network network) {
    return {network, id};
  }

  Address Address::makeSecp256k1(const Sec256k1PublicKey &public_key,
                                 Network network) {
    return {network, Secp256k1PublicKeyHash{blake2b_160(public_key)}};
  }

  Address Address::makeActorExec(gsl::span<const uint8_t> data,
                                 Network network) {
    return {network, ActorExecHash{blake2b_160(data)}};
  }

  Address Address::makeBls(const BlsPublicKey &public_key, Network network) {
    return {network, BLSPublicKeyHash{public_key}};
  }

  uint64_t Address::getId() const {
    return boost::get<uint64_t>(data);
  }

  bool Address::verifySyntax(gsl::span<const uint8_t> seed_data) const {
    return visit_in_place(
        data,
        [](uint64_t v) { return true; },
        [&seed_data](const Secp256k1PublicKeyHash &v) {
          if (seed_data.size() != libp2p::crypto::secp256k1::kPublicKeyLength) {
            return false;
          }
          auto hash = blake2b_160(seed_data);
          return std::equal(v.begin(), v.end(), hash.begin());
        },
        [&seed_data](const ActorExecHash &v) {
          auto hash = blake2b_160(seed_data);
          return std::equal(v.begin(), v.end(), hash.begin());
        },
        [&seed_data](const BLSPublicKeyHash &v) {
          if (seed_data.size() != std::tuple_size<BlsPublicKey>::value) {
            return false;
          }
          return std::equal(v.begin(), v.end(), seed_data.begin());
        });
  }

  bool operator==(const Address &lhs, const Address &rhs) {
    return lhs.network == rhs.network && lhs.data == rhs.data;
  }

  bool operator!=(const Address &lhs, const Address &rhs) {
    return !(lhs == rhs);
  }

  bool operator<(const Address &lhs, const Address &rhs) {
    return lhs.network < rhs.network
           || (lhs.network == rhs.network
               && (lhs.getProtocol() < rhs.getProtocol()
                   || (lhs.getProtocol() == rhs.getProtocol()
                       && lhs.data < rhs.data)));
  }

};  // namespace fc::primitives::address
