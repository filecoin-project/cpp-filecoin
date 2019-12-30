/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/impl/message_signer_bls.hpp"
#include "common/which.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"

namespace fc::vm::message {

  using fc::crypto::bls::impl::BlsProviderImpl;
  using BlsPrivateKey = fc::crypto::bls::PrivateKey;
  using BlsPublicKey = fc::crypto::bls::PublicKey;

  BlsMessageSigner::BlsMessageSigner()
      : bls_provider_(std::make_shared<BlsProviderImpl>()) {}
  BlsMessageSigner::BlsMessageSigner(std::shared_ptr<BlsProvider> bls_provider)
      : bls_provider_(bls_provider) {}

  outcome::result<SignedMessage> BlsMessageSigner::sign(
      const UnsignedMessage &msg, const PrivateKey &key) noexcept {
    if (key.size() != kBlsPrivateKeyLength) {
      return outcome::failure(MessageError::INVALID_KEY_LENGTH);
    }
    BlsPrivateKey privateKey{};
    std::copy_n(key.begin(), kBlsPrivateKeyLength, privateKey.begin());

    OUTCOME_TRY(
        raw_bytes,
        bls_provider_->sign(serialize<UnsignedMessage>(msg), privateKey));

    Signature::BlsSignature signature{};
    std::copy_n(std::make_move_iterator(raw_bytes.begin()),
                kBlsSignatureLength,
                signature.begin());

    return SignedMessage{msg, Signature{Signature::BLS, signature}};
  }

  outcome::result<bool> BlsMessageSigner::verify(
      const SignedMessage &msg, const PublicKey &key) noexcept {
    if (msg.signature.type != Signature::BLS
        || !common::which<Signature::BlsSignature>(msg.signature.data)) {
      return outcome::failure(MessageError::WRONG_SIGNATURE_TYPE);
    }
    if (key.size() != kBlsPublicKeyLength) {
      return outcome::failure(MessageError::INVALID_KEY_LENGTH);
    }
    BlsPublicKey publicKey{};
    std::copy_n(key.begin(), kBlsPublicKeyLength, publicKey.begin());

    auto signature_bytes =
        boost::get<Signature::BlsSignature>(msg.signature.data);
    return bls_provider_->verifySignature(
        serialize<UnsignedMessage>(msg.message), signature_bytes, publicKey);
    ;
  }

};  // namespace fc::vm::message