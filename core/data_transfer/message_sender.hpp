/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_MESSAGE_SENDER_HPP
#define CPP_FILECOIN_DATA_TRANSFER_MESSAGE_SENDER_HPP

#include "data_transfer/message.hpp"

namespace fc::data_transfer {

  /**
   * MessageSender is an interface to send messages to a peer
   */
  class MessageSender {
   public:
    virtual ~MessageSender() = default;

    virtual outcome::result<void> sendMessage(
        const DataTransferMessage &message) = 0;

    virtual outcome::result<void> close() = 0;

    virtual outcome::result<void> reset() = 0;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_MESSAGE_SENDER_HPP
