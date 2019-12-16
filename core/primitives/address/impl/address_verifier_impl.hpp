/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_VERIFIER_IMPL_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_VERIFIER_IMPL_HPP

#include "primitives/address/address_verifier.hpp"

namespace fc::primitives::address {

  class AddressVerifierImpl : public AddressVerifier {
   public:
    /** @copydoc AddressVerifier::verifySyntax() */
    fc::outcome::result<bool> verifySyntax(
        const Address &address,
        gsl::span<const uint8_t> seed_data) noexcept override;
  };

}  // namespace fc::primitives::address

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_ADDRESS_VERIFIER_IMPL_HPP
