/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "crypto/bls/bls_provider.hpp"

namespace fc::crypto::bls {
  class BlsProviderMock : public BlsProvider {
   public:
    MOCK_CONST_METHOD0(generateKeyPair, outcome::result<KeyPair>());

    MOCK_CONST_METHOD1(derivePublicKey,
                       outcome::result<PublicKey>(const PrivateKey &));

    MOCK_CONST_METHOD2(sign,
                       outcome::result<Signature>(gsl::span<const uint8_t>,
                                                  const PrivateKey &));

    MOCK_CONST_METHOD3(verifySignature,
                       outcome::result<bool>(gsl::span<const uint8_t>,
                                             const Signature &,
                                             const PublicKey &));
    MOCK_CONST_METHOD1(
        aggregateSignatures,
        outcome::result<Signature>(gsl::span<const Signature>));
  };
}  // namespace fc::crypto::bls
