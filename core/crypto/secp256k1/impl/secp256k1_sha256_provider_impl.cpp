/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"

#include <libp2p/common/types.hpp>
#include "crypto/sha/sha256.hpp"

namespace fc::crypto::secp256k1 {
  using sha::Hash256;
  using sha::sha256;

  outcome::result<SignatureCompact> Secp256k1Sha256ProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &key) const {
    Hash256 digest = sha256(message);
    return Secp256k1ProviderImpl::sign(digest, key);
  }

  outcome::result<bool> Secp256k1Sha256ProviderImpl::verify(
      gsl::span<const uint8_t> message,
      const SignatureCompact &signature,
      const PublicKeyUncompressed &key) const {
    Hash256 digest = sha256(message);
    return Secp256k1ProviderImpl::verify(digest, signature, key);
  }

  outcome::result<PublicKeyUncompressed>
  Secp256k1Sha256ProviderImpl::recoverPublicKey(
      gsl::span<const uint8_t> message,
      const SignatureCompact &signature) const {
    Hash256 digest = sha256(message);
    return Secp256k1ProviderImpl::recoverPublicKey(digest, signature);
  }

}  // namespace fc::crypto::secp256k1
