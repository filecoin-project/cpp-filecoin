/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/message.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::message, MessageError, e) {
  using fc::vm::message::MessageError;
  switch (e) {
    case (MessageError::INVALID_LENGTH):
      return "MessageError: invalid message length";
    case (MessageError::SERIALIZATION_FAILURE):
      return "MessageError: message serialization failed";
    case (MessageError::VERIFICATION_FAILURE):
      return "MessageError: signature verification failed";
    default:
      return "Unknown error";
  };
}
