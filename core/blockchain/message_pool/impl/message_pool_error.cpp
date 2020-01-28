/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/message_pool/message_pool_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::message_pool, MessagePoolError, e) {
  using fc::blockchain::message_pool::MessagePoolError;

  switch (e) {
    case MessagePoolError::MESSAGE_ALREADY_IN_POOL:
      return "MessagePoolError: message is already in pool";
  }

  return "unknown error";
}
