/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "crypto/secp256k1/secp256k1_provider.hpp"

namespace fc::crypto::secp256k1 {

  class Secp256k1ProviderMock : public Secp256k1ProviderDefault {
   public:
    MOCK_CONST_METHOD0(generate, outcome::result<KeyPair>());

    MOCK_CONST_METHOD1(derive, outcome::result<PublicKey>(const PrivateKey &));

    MOCK_CONST_METHOD2(sign,
                       outcome::result<Signature>(gsl::span<const uint8_t>,
                                                  const PrivateKey &));

    MOCK_CONST_METHOD3(verify,
                       outcome::result<bool>(gsl::span<const uint8_t>,
                                             const Signature &,
                                             const PublicKey &));

    MOCK_CONST_METHOD2(recoverPublicKey,
                       outcome::result<PublicKey>(gsl::span<const uint8_t>,
                                                  const Signature &));
  };

}  // namespace fc::crypto::secp256k1
