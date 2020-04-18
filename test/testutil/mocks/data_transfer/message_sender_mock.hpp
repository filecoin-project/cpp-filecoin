/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESTUTIL_MOCKS_MESSAGE_SENDER_MOCK_HPP
#define CPP_FILECOIN_TESTUTIL_MOCKS_MESSAGE_SENDER_MOCK_HPP

#include "data_transfer/message_sender.hpp"

#include <gmock/gmock.h>

namespace fc::data_transfer {

  class MessageSenderMock : public MessageSender {
   public:
    MOCK_METHOD1(sendMessage,
                 outcome::result<void>(const DataTransferMessage &message));

    MOCK_METHOD0(close, outcome::result<void>());

    MOCK_METHOD0(reset, outcome::result<void>());
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_TESTUTIL_MOCKS_MESSAGE_SENDER_MOCK_HPP
