/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bls/impl/bls_provider_impl.hpp"

#include <filecoin-ffi/filcrypto.h>
#include <gsl/gsl_util>

#include "common/ffi.hpp"

namespace fc::crypto::bls {
  namespace ffi = common::ffi;

  outcome::result<KeyPair> BlsProviderImpl::generateKeyPair() const {
    fil_PrivateKeyGenerateResponse *private_key_response = fil_private_key_generate();
    if (private_key_response == nullptr) {
      return Errors::KeyPairGenerationFailed;
    }
    auto free_private_response = gsl::finally([private_key_response]() {
      fil_destroy_private_key_generate_response(private_key_response);
    });
    auto private_key = ffi::array(private_key_response->private_key.inner);
    OUTCOME_TRY(public_key, derivePublicKey(private_key));
    return KeyPair{private_key, public_key};
  }

  outcome::result<PublicKey> BlsProviderImpl::derivePublicKey(
      const PrivateKey &key) const {
    fil_PrivateKeyPublicKeyResponse *response = fil_private_key_public_key(key.data());
    if (response == nullptr) {
      return Errors::InvalidPrivateKey;
    }
    auto free_response = gsl::finally(
        [response]() { fil_destroy_private_key_public_key_response(response); });
    return ffi::array(response->public_key.inner);
  }

  outcome::result<Signature> BlsProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &key) const {
    fil_PrivateKeySignResponse *response =
        fil_private_key_sign(key.data(), message.data(), message.size());
    if (response == nullptr) {
      return Errors::SignatureGenerationFailed;
    }
    auto free_response = gsl::finally(
        [response]() { fil_destroy_private_key_sign_response(response); });
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
             != 0;
  }

  outcome::result<Digest> BlsProviderImpl::generateHash(
      gsl::span<const uint8_t> message) {
    fil_HashResponse *response = fil_hash(message.data(), message.size());
    if (response == nullptr) {
      return Errors::InternalError;
    }
    auto free_response =
        gsl::finally([response]() { fil_destroy_hash_response(response); });
    return ffi::array(response->digest.inner);
  }

  outcome::result<Signature> BlsProviderImpl::aggregateSignatures(
      gsl::span<const Signature> signatures) const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const uint8_t *flat_bytes =
        reinterpret_cast<const uint8_t *>(signatures.data());
    size_t flat_size = sizeof(Signature) * signatures.size();
    fil_AggregateResponse *response = fil_aggregate(flat_bytes, flat_size);
    if (response == nullptr) {
      return Errors::InternalError;
    }
    auto free_response =
        gsl::finally([response]() { fil_destroy_aggregate_response(response); });
    return ffi::array(response->signature.inner);
  }
};  // namespace fc::crypto::bls

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::bls, Errors, e) {
  using fc::crypto::bls::Errors;
  switch (e) {
    case (Errors::InternalError):
      return "BLS provider: internal error";
    case (Errors::InvalidPrivateKey):
      return "BLS provider: invalid private key";
    case (Errors::InvalidPublicKey):
      return "BLS provider:: invalid public key";
    case (Errors::KeyPairGenerationFailed):
      return "BLS provider: key pair generation failed";
    case (Errors::SignatureGenerationFailed):
      return "BLS provider: signature generation failed";
    case (Errors::SignatureVerificationFailed):
      return "BLS provider: signature verification failed";
    case (Errors::AggregateError):
      return "BLS provider: signatures aggregating failed";
    default:
      return "BLS provider: unknown error";
  }
}
