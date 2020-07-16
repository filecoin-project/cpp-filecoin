/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include "crypto/secp256k1/secp256k1_error.hpp"
#include "secp256k1_recovery.h"

namespace fc::crypto::secp256k1 {

  Secp256k1ProviderImpl::Secp256k1ProviderImpl()
      : context_(secp256k1_context_create(SECP256K1_CONTEXT_SIGN
                                          | SECP256K1_CONTEXT_VERIFY),
                 secp256k1_context_destroy) {}

  outcome::result<KeyPair> Secp256k1ProviderImpl::generate() const {
    PublicKeyUncompressed public_key{};
    PrivateKey private_key{};
    std::shared_ptr<EC_KEY> key{EC_KEY_new_by_curve_name(NID_secp256k1),
                                EC_KEY_free};
    if (EC_KEY_generate_key(key.get()) != 1) {
      return Secp256k1Error::kKeyGenerationFailed;
    }
    const BIGNUM *privateNum = EC_KEY_get0_private_key(key.get());
    if (BN_bn2binpad(privateNum, private_key.data(), private_key.size()) < 0) {
      return Secp256k1Error::kKeyGenerationFailed;
    }
    EC_KEY_set_conv_form(key.get(), POINT_CONVERSION_UNCOMPRESSED);
    auto public_key_length = i2o_ECPublicKey(key.get(), nullptr);
    if (public_key_length != public_key.size()) {
      return Secp256k1Error::kKeyGenerationFailed;
    }
    uint8_t *public_key_ptr = public_key.data();
    if (i2o_ECPublicKey(key.get(), &public_key_ptr) != public_key_length) {
      return Secp256k1Error::kKeyGenerationFailed;
    }
    return KeyPair{private_key, public_key};
  }

  outcome::result<PublicKeyUncompressed> Secp256k1ProviderImpl::derive(
      const PrivateKey &key) const {
    secp256k1_pubkey pubkey;

    if (!secp256k1_ec_pubkey_create(context_.get(), &pubkey, key.data())) {
      return Secp256k1Error::kKeyGenerationFailed;
    }

    PublicKeyUncompressed public_key{};
    size_t outputlen = kPublicKeyUncompressedLength;
    if (!secp256k1_ec_pubkey_serialize(context_.get(),
                                       public_key.data(),
                                       &outputlen,
                                       &pubkey,
                                       SECP256K1_EC_UNCOMPRESSED)) {
      return Secp256k1Error::kPubkeySerializationError;
    }

    return public_key;
  }

  outcome::result<PublicKeyUncompressed> Secp256k1ProviderImpl::sign(
      gsl::span<const uint8_t> message, const PrivateKey &key) const {
    secp256k1_ecdsa_recoverable_signature sig_struct;
    if (!secp256k1_ecdsa_sign_recoverable(context_.get(),
                                          &sig_struct,
                                          message.data(),
                                          key.cbegin(),
                                          secp256k1_nonce_function_rfc6979,
                                          nullptr)) {
      return Secp256k1Error::kCannotSignError;
    }
    SignatureCompact signature{};
    int recid = 0;
    if (!secp256k1_ecdsa_recoverable_signature_serialize_compact(
            context_.get(), signature.data(), &recid, &sig_struct)) {
      return Secp256k1Error::kSignatureSerializationError;
    }
    signature[64] = (uint8_t)recid;
    return signature;
  }

  outcome::result<bool> Secp256k1ProviderImpl::verify(
      gsl::span<const uint8_t> message,
      const SignatureCompact &signature,
      const PublicKeyUncompressed &key) const {
    OUTCOME_TRY(checkSignature(signature));

    secp256k1_ecdsa_signature sig;
    secp256k1_pubkey pubkey;

    if (!secp256k1_ecdsa_signature_parse_compact(
            context_.get(), &sig, signature.data())) {
      return Secp256k1Error::kSignatureParseError;
    }
    if (!secp256k1_ec_pubkey_parse(
            context_.get(), &pubkey, key.data(), key.size())) {
      return Secp256k1Error::kPubkeyParseError;
    }
    return (secp256k1_ecdsa_verify(
        context_.get(), &sig, message.data(), &pubkey) == 1);
  }

  outcome::result<PublicKeyUncompressed>
  Secp256k1ProviderImpl::recoverPublicKey(
      gsl::span<const uint8_t> message,
      const SignatureCompact &signature) const {
    OUTCOME_TRY(checkSignature(signature));

    secp256k1_ecdsa_recoverable_signature sig_rec;
    secp256k1_pubkey pubkey;

    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
            context_.get(), &sig_rec, signature.data(), (int)signature[64])) {
      return Secp256k1Error::kSignatureParseError;
    }
    if (!secp256k1_ecdsa_recover(
            context_.get(), &pubkey, &sig_rec, message.data())) {
      return Secp256k1Error::kRecoverError;
    }
    PublicKeyUncompressed pubkey_out;
    size_t outputlen = kPublicKeyUncompressedLength;
    if (!secp256k1_ec_pubkey_serialize(context_.get(),
                                       pubkey_out.data(),
                                       &outputlen,
                                       &pubkey,
                                       SECP256K1_EC_UNCOMPRESSED)) {
      return Secp256k1Error::kPubkeySerializationError;
    }

    return pubkey_out;
  }

  outcome::result<void> Secp256k1ProviderImpl::checkSignature(
      const SignatureCompact &signature) const {
    if (signature[64] > 3) {
      return Secp256k1Error::kSignatureParseError;
    }
    return outcome::success();
  }

}  // namespace fc::crypto::secp256k1
