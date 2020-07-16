/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bls/impl/bls_provider_impl.hpp"

#include <filecoin-ffi/filcrypto.h>

#include "common/ffi.hpp"
#include "common/span.hpp"

namespace fc::crypto::bls {
  namespace ffi = common::ffi;

  outcome::result<KeyPair> BlsProviderImpl::generateKeyPair() const {
    auto response{ffi::wrap(fil_private_key_generate(),
                            fil_destroy_private_key_generate_response)};
    if (response == nullptr) {
      return Errors::kKeyPairGenerationFailed;
    }
    auto private_key = ffi::array(response->private_key.inner);
    OUTCOME_TRY(public_key, derivePublicKey(private_key));
    return KeyPair{private_key, public_key};
  }

  outcome::result<PublicKey> BlsProviderImpl::derivePublicKey(
      const PrivateKey &key) const {
    auto response{ffi::wrap(fil_private_key_public_key(key.data()),
                            fil_destroy_private_key_public_key_response)};
    if (response == nullptr) {
      return Errors::kInvalidPrivateKey;
    }
    return ffi::array(response->public_key.inner);
  }

  outcome::result<Signature> BlsProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &key) const {
    auto response{ffi::wrap(
        fil_private_key_sign(key.data(), message.data(), message.size()),
        fil_destroy_private_key_sign_response)};
    if (response == nullptr) {
      return Errors::kSignatureGenerationFailed;
    }
    return ffi::array(response->signature.inner);
  }

  outcome::result<bool> BlsProviderImpl::verifySignature(
      gsl::span<const uint8_t> message,
      const Signature &signature,
      const PublicKey &key) const {
    OUTCOME_TRY(digest, generateHash(message));
    return fil_verify(signature.data(),
                      digest.data(),
                      digest.size(),
                      key.data(),
                      key.size())
           > 0;
  }

  outcome::result<Digest> BlsProviderImpl::generateHash(
      gsl::span<const uint8_t> message) {
    auto response{ffi::wrap(fil_hash(message.data(), message.size()),
                            fil_destroy_hash_response)};
    if (response == nullptr) {
      return Errors::kInternalError;
    }
    return ffi::array(response->digest.inner);
  }

  outcome::result<Signature> BlsProviderImpl::aggregateSignatures(
      gsl::span<const Signature> signatures) const {
    auto response{ffi::wrap(
        fil_aggregate(common::span::cast<const uint8_t>(signatures).data(),
                      signatures.size_bytes()),
        fil_destroy_aggregate_response)};
    if (response == nullptr) {
      return Errors::kInternalError;
    }
    return ffi::array(response->signature.inner);
  }
};  // namespace fc::crypto::bls

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::bls, Errors, e) {
  using fc::crypto::bls::Errors;
  switch (e) {
    case (Errors::kInternalError):
      return "BLS provider: internal error";
    case (Errors::kInvalidPrivateKey):
      return "BLS provider: invalid private key";
    case (Errors::kInvalidPublicKey):
      return "BLS provider:: invalid public key";
    case (Errors::kKeyPairGenerationFailed):
      return "BLS provider: key pair generation failed";
    case (Errors::kSignatureGenerationFailed):
      return "BLS provider: signature generation failed";
    case (Errors::kSignatureVerificationFailed):
      return "BLS provider: signature verification failed";
    case (Errors::kAggregateError):
      return "BLS provider: signatures aggregating failed";
    default:
      return "BLS provider: unknown error";
  }
}
