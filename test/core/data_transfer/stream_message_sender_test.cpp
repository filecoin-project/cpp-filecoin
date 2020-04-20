/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/stream_message_sender.hpp"

#include <gtest/gtest.h>
#include <mock/libp2p/connection/stream_mock.hpp>
#include "testutil/outcome.hpp"

using libp2p::connection::StreamMock;
using ::testing::_;
using ::testing::Eq;

namespace fc::data_transfer {
  class StreamMessageSenderTest : public ::testing::Test {
   public:
    std::shared_ptr<StreamMock> stream = std::make_shared<StreamMock>();
    StreamMessageSender stream_message_sender{stream};
  };

  /**
   * @given message sender
   * @when sent message
   * @then Message encoded and sent
   */
  TEST_F(StreamMessageSenderTest, SendMessage) {
    DataTransferResponse response{.is_accepted = true, .transfer_id = 1};
    DataTransferMessage message{
        .is_request = false, .request = boost::none, .response = response};
    EXPECT_CALL(*stream, write(_, _, _)).WillOnce(testing::Return());
    EXPECT_OUTCOME_TRUE_1(stream_message_sender.sendMessage(message));
  }

  /**
   * Close message sender
   */
  TEST_F(StreamMessageSenderTest, Close) {
    EXPECT_CALL(*stream, close(_)).WillOnce(testing::Return());
    EXPECT_OUTCOME_TRUE_1(stream_message_sender.close());
  }

}  // namespace fc::data_transfer
