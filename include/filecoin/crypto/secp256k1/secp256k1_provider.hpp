/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_CRYPTO_SECP256K1_PROVIDER_SECP256K1_PROVIDER_HPP
#define CPP_FILECOIN_TEST_CORE_CRYPTO_SECP256K1_PROVIDER_SECP256K1_PROVIDER_HPP

#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>

namespace fc::crypto::secp256k1 {
  using libp2p::crypto::secp256k1::KeyPair;
  using libp2p::crypto::secp256k1::kPrivateKeyLength;
  using libp2p::crypto::secp256k1::kPublicKeyLength;
  using libp2p::crypto::secp256k1::PrivateKey;
  using libp2p::crypto::secp256k1::PublicKey;
  using libp2p::crypto::secp256k1::Secp256k1Provider;
  using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;
  using libp2p::crypto::secp256k1::Signature;
}  // namespace fc::crypto::secp256k1

#endif  // CPP_FILECOIN_TEST_CORE_CRYPTO_SECP256K1_PROVIDER_SECP256K1_PROVIDER_HPP
