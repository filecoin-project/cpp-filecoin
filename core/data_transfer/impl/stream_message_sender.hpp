/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_STREAM_MESSAGE_SENDER_HPP
#define CPP_FILECOIN_DATA_TRANSFER_STREAM_MESSAGE_SENDER_HPP

#include "data_transfer/message_sender.hpp"

#include <libp2p/connection/stream.hpp>

namespace fc::data_transfer {

  class StreamMessageSender : public MessageSender {
   public:
    explicit StreamMessageSender(
        std::shared_ptr<libp2p::connection::Stream> stream);

    outcome::result<void> sendMessage(
        const DataTransferMessage &message) override;

    outcome::result<void> close() override;

    outcome::result<void> reset() override;

   private:
    std::shared_ptr<libp2p::connection::Stream> stream_;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_STREAM_MESSAGE_SENDER_HPP
