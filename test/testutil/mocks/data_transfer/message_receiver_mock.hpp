/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MESSAGE_RECEIVER_MOCK_HPP
#define CPP_FILECOIN_MESSAGE_RECEIVER_MOCK_HPP

#include "data_transfer/message_receiver.hpp"

#include <gmock/gmock.h>

namespace fc::data_transfer {

  class MessageReceiverMock : public MessageReceiver {
   public:
    ~MessageReceiverMock() override = default;

    MOCK_METHOD2(receiveRequest,
                 outcome::result<void>(const PeerInfo &initiator,
                                       const DataTransferRequest &request));
    MOCK_METHOD2(receiveResponse,
                 outcome::result<void>(const PeerInfo &sender,
                                       const DataTransferResponse &response));
    MOCK_METHOD0(receiveError, void());
    MOCK_METHOD2(
        registerVoucherType,
        outcome::result<void>(const std::string &type,
                              std::shared_ptr<RequestValidator> validator));
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_MESSAGE_RECEIVER_MOCK_HPP
