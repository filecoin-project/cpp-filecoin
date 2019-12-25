/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bls/impl/bls_provider_impl.hpp"

#include <gsl/gsl_util>

namespace fc::crypto::bls::impl {
  outcome::result<KeyPair> BlsProviderImpl::generateKeyPair() const {
    KeyPair key_pair{};
    PrivateKeyGenerateResponse *private_key_response = private_key_generate();
    if (private_key_response == nullptr) {
      return Errors::KeyPairGenerationFailed;
    }
    auto free_private_response = gsl::finally([private_key_response]() {
      destroy_private_key_generate_response(private_key_response);
    });
    std::move(std::begin(private_key_response->private_key),
              std::end(private_key_response->private_key),
              key_pair.private_key.begin());
    OUTCOME_TRY(public_key, derivePublicKey(key_pair.private_key));
    key_pair.public_key = public_key;
    return key_pair;
  }

  outcome::result<PublicKey> BlsProviderImpl::derivePublicKey(
      const PrivateKey &key) const {
    PublicKey public_key;
    PrivateKeyPublicKeyResponse *response = private_key_public_key(key.data());
    if (response == nullptr) {
      return Errors::InvalidPrivateKey;
    }
    auto free_response = gsl::finally(
        [response]() { destroy_private_key_public_key_response(response); });
    std::move(std::begin(response->public_key),
              std::end(response->public_key),
              public_key.begin());
    return public_key;
  }

  outcome::result<Signature> BlsProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &key) const {
    Signature signature;
    PrivateKeySignResponse *response =
        private_key_sign(key.data(), message.data(), message.size());
    if (response == nullptr) {
      return Errors::SignatureGenerationFailed;
    }
    auto free_response = gsl::finally(
        [response]() { destroy_private_key_sign_response(response); });
    std::move(std::begin(response->signature),
              std::end(response->signature),
              signature.begin());
    return signature;
  }

  outcome::result<bool> BlsProviderImpl::verifySignature(
      gsl::span<const uint8_t> message,
      const Signature &signature,
      const PublicKey &key) const {
    OUTCOME_TRY(digest, generateHash(message));
    if (verify(signature.data(),
               digest.data(),
               digest.size(),
               key.data(),
               key.size())
        != 0) {
      return true;
    }
    return false;
  }

  outcome::result<Digest> BlsProviderImpl::generateHash(
      gsl::span<const uint8_t> message) {
    Digest digest;
    HashResponse *response = hash(message.data(), message.size());
    if (response == nullptr) {
      return Errors::InternalError;
    }
    auto free_response =
        gsl::finally([response]() { destroy_hash_response(response); });
    std::move(std::begin(response->digest),
              std::end(response->digest),
              digest.begin());
    return digest;
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
    default:
      return "BLS provider: unknown error";
  }
}
