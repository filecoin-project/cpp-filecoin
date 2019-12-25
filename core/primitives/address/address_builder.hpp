/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_BUILDER_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_BUILDER_HPP

#include "crypto/secp256k1/secp256k1_provider.hpp"

#include "common/outcome.hpp"
#include "crypto/bls/bls_types.hpp"
#include "address.hpp"

namespace fc::primitives::address {

  /**
   * Interface of address builder.
   */
  class AddressBuilder {
   public:
    virtual ~AddressBuilder() = default;

    /**
     * @brief create address form Secp256k1 public key
     * @param public_key - Secp256k1 public key
     * @return address created from secp256k1 public key
     */
    virtual outcome::result<Address> makeFromSecp256k1PublicKey(
        Network network,
        const crypto::secp256k1::PublicKey &public_key) noexcept = 0;

    /**
     * @brief create address form BLS public key
     * @param public_key - BLS public key
     * @return address created from BLS public key
     */
    virtual outcome::result<Address> makeFromBlsPublicKey(
        Network network, const crypto::bls::PublicKey &public_key) noexcept = 0;
  };

}  // namespace fc::primitives::address

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_BUILDER_HPP
