/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/address.hpp"

#include "common/visitor.hpp"
#include "crypto/blake2/blake2b160.hpp"

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

  Address Address::makeFromId(uint64_t id) {
    // TODO(turuslan): FIL-118 remove hardcoded TESTNET
    return {TESTNET, id};
  }

  Address Address::makeActorExecAddress(gsl::span<const uint8_t> data) {
    // TODO(turuslan): FIL-118 remove hardcoded TESTNET
    return {TESTNET, ActorExecHash{blake2b_160(data).value()}};
  }

  bool operator==(const Address &lhs, const Address &rhs) {
    return lhs.network == rhs.network && lhs.getProtocol() == rhs.getProtocol()
           && lhs.data == rhs.data;
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
