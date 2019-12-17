/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_BUILDER_IMPL_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_BUILDER_IMPL_HPP

#include "primitives/address/address_builder.hpp"

namespace fc::primitives::address {

  class AddressBuilderImpl : public AddressBuilder {
   public:
    /** @copydoc AddressBuilder::makeFromSecp256k1PublicKey() */
    outcome::result<Address> makeFromSecp256k1PublicKey(
        Network network,
        const libp2p::crypto::secp256k1::PublicKey
            &public_key) noexcept override;

    /** @copydoc AddressBuilder::makeFromBlsPublicKey() */
    outcome::result<Address> makeFromBlsPublicKey(
        Network network,
        const crypto::bls::PublicKey &public_key) noexcept override;
  };

}  // namespace fc::primitives::address

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_BUILDER_IMPL_HPP
