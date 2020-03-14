/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/impl/message_signer_impl.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>

#include "vm/message/message_util.hpp"

namespace fc::vm::message {

  MessageSignerImpl::MessageSignerImpl(std::shared_ptr<KeyStore> ks)
      : keystore_(std::move(ks)),
        logger_(common::createLogger("message_signer")) {}

  outcome::result<SignedMessage> MessageSignerImpl::sign(
      const Address &address, const UnsignedMessage &msg) noexcept {
    auto maybe_cid = cid(msg);
    if (maybe_cid.has_error()) {
      logger_->error(maybe_cid.error().message());
      return outcome::failure(MessageError::SERIALIZATION_FAILURE);
    }
    auto maybe_cid_bytes =
        libp2p::multi::ContentIdentifierCodec::encode(maybe_cid.value());
    if (maybe_cid_bytes.has_error()) {
      logger_->error(maybe_cid_bytes.error().message());
      return outcome::failure(MessageError::SERIALIZATION_FAILURE);
    }
    OUTCOME_TRY(signature, keystore_->sign(address, maybe_cid_bytes.value()));
    return SignedMessage{msg, signature};
  }

  outcome::result<UnsignedMessage> MessageSignerImpl::verify(
      const Address &address, const SignedMessage &msg) const noexcept {
    auto maybe_cid = cid(msg.message);
    if (maybe_cid.has_error()) {
      logger_->error(maybe_cid.error().message());
      return outcome::failure(MessageError::SERIALIZATION_FAILURE);
    }
    auto maybe_cid_bytes =
        libp2p::multi::ContentIdentifierCodec::encode(maybe_cid.value());
    if (maybe_cid_bytes.has_error()) {
      logger_->error(maybe_cid_bytes.error().message());
      return outcome::failure(MessageError::SERIALIZATION_FAILURE);
    }
    OUTCOME_TRY(
        res,
        keystore_->verify(address, maybe_cid_bytes.value(), msg.signature));
    if (!res) {
      return outcome::failure(MessageError::VERIFICATION_FAILURE);
    }
    return msg.message;
  }

};  // namespace fc::vm::message
