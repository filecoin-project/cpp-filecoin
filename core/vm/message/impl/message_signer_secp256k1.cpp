/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/impl/message_signer_secp256k1.hpp"
#include "common/which.hpp"

namespace fc::vm::message {

  using fc::crypto::secp256k1::Secp256k1ProviderImpl;
  using Secp256k1PrivateKey = fc::crypto::secp256k1::PrivateKey;
  using Secp256k1PublicKey = fc::crypto::secp256k1::PublicKey;

  Secp256k1MessageSigner::Secp256k1MessageSigner()
      : secp256k1_provider_(std::make_shared<Secp256k1ProviderImpl>()) {}
  Secp256k1MessageSigner::Secp256k1MessageSigner(
      std::shared_ptr<Secp256k1Provider> secp256k1_provider)
      : secp256k1_provider_(std::move(secp256k1_provider)) {}

  outcome::result<SignedMessage> Secp256k1MessageSigner::sign(
      const UnsignedMessage &msg, const PrivateKey &key) noexcept {
    if (static_cast<uint64_t>(key.size())
        != crypto::secp256k1::kPrivateKeyLength) {
      return outcome::failure(MessageError::INVALID_KEY_LENGTH);
    }
    Secp256k1PrivateKey privateKey{};
    std::copy_n(
        key.begin(), crypto::secp256k1::kPrivateKeyLength, privateKey.begin());

    OUTCOME_TRY(
        signature,
        secp256k1_provider_->sign(serialize<UnsignedMessage>(msg), privateKey));

    return SignedMessage{msg,
                         Signature{Signature::SECP256K1,
                                   Signature::Secp256k1Signature(
                                       signature.begin(), signature.end())}};
  }

  outcome::result<bool> Secp256k1MessageSigner::verify(
      const SignedMessage &msg, const PublicKey &key) noexcept {
    if (msg.signature.type != Signature::SECP256K1
        || !common::which<Signature::Secp256k1Signature>(msg.signature.data)) {
      return outcome::failure(MessageError::WRONG_SIGNATURE_TYPE);
    }
    if (static_cast<uint64_t>(key.size())
        != crypto::secp256k1::kPublicKeyLength) {
      return outcome::failure(MessageError::INVALID_KEY_LENGTH);
    }
    Secp256k1PublicKey publicKey{};
    std::copy_n(
        key.begin(), crypto::secp256k1::kPublicKeyLength, publicKey.begin());

    auto bytes = boost::get<Signature::Secp256k1Signature>(msg.signature.data);
    return secp256k1_provider_->verify(
        serialize<UnsignedMessage>(msg.message), bytes, publicKey);
    ;
  }

};  // namespace fc::vm::message