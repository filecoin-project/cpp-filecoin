/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_VERIFIER_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_VERIFIER_HPP

#include "common/outcome.hpp"
#include "primitives/address.hpp"

namespace fc::primitives::address {

  /**
   * Verify if seed_data is a base for address. If address is:
   * 0 - id - is always valid
   * 1 - sec256k1 - check payload field contains the Blake2b 160 hash of the
   * public key
   * 2 - actor - check payload field is Blake2b 160 hash of the meaningful
   * data
   * 3 - bls - check payload is a BLS public key
   * @param address - address to verify
   * @param seed_data - data to generate address
   * @return true if data is a base for address
   */
  fc::outcome::result<bool> verifySyntax(
      const Address &address, gsl::span<const uint8_t> seed_data) noexcept;

};  // namespace fc::primitives::address

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_VERIFIER_HPP
