/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address.hpp"

#include "common/visitor.hpp"

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
    return visit_in_place(data,
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
