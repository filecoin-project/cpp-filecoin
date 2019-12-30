/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/message.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::message, MessageError, e) {
  using fc::vm::message::MessageError;
  switch (e) {
    case (MessageError::INVALID_LENGTH):
      return "Invalid message length";
    case (MessageError::INVALID_KEY_LENGTH):
      return "Invalid private/public key length";
    case (MessageError::INVALID_SIGNATURE_LENGTH):
      return "Invalid signature length";
    case (MessageError::WRONG_SIGNATURE_TYPE):
      return "Unsupported signature type";
    default:
      return "Unknown error";
  };
}

namespace fc::vm::message {
  const uint64_t kMessageMaxSize = 32 * 1024;
  const uint64_t kSignatureMaxLength = 200;
  const uint64_t kBlsSignatureLength = 96;
  const uint64_t kBlsPrivateKeyLength = 32;
  const uint64_t kBlsPublicKeyLength = 48;

};  // namespace fc::vm::message
