/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/libp2p_data_transfer_network.hpp"

#include <gtest/gtest.h>
#include <libp2p/basic/message_read_writer_error.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <mock/libp2p/connection/stream_mock.hpp>
#include <mock/libp2p/host/host_mock.hpp>
#include "testutil/mocks/data_transfer/message_receiver_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/peer_id.hpp"

using fc::data_transfer::createRequest;
using fc::data_transfer::createResponse;
using fc::data_transfer::DataTransferMessage;
using fc::data_transfer::DataTransferNetwork;
using fc::data_transfer::kDataTransferLibp2pProtocol;
using fc::data_transfer::Libp2pDataTransferNetwork;
using fc::data_transfer::MessageReceiver;
using fc::data_transfer::MessageReceiverMock;
using fc::data_transfer::TransferId;
using ReadCallbackFunc = libp2p::basic::Reader::ReadCallbackFunc;
using libp2p::connection::Stream;
using libp2p::multi::Multiaddress;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;
using libp2p::peer::Protocol;
using ::testing::_;
using ::testing::Eq;

class Libp2pDataTransferNetworkTest : public ::testing::Test {
 public:
  /**
   * Imitates network.setDelegate(Protocol, Handler) call and captures handler
   * for further inspection
   */
  void captureProtocolHandler(const Protocol &,
                              const std::function<Stream::Handler> &handler) {
    protocol_handler_ = handler;
  }

  /**
   * Protocol handler callback
   */
  std::function<Stream::Handler> protocol_handler_;

  std::shared_ptr<libp2p::HostMock> host = std::make_shared<libp2p::HostMock>();
  std::shared_ptr<DataTransferNetwork> network =
      std::make_shared<Libp2pDataTransferNetwork>(host);
  Multiaddress multiaddress =
      Multiaddress::create("/ip4/127.0.0.1/tcp/40005").value();
};

/**
 * Successful connect to peer
 */
TEST_F(Libp2pDataTransferNetworkTest, Connect) {
  PeerInfo peer_info{.id = generatePeerId(1), .addresses = {}};
  EXPECT_CALL(*host, connect(Eq(peer_info))).WillOnce(testing::Return());
  EXPECT_OUTCOME_TRUE_1(network->connectTo(peer_info));
}

/**
 * Set protocol receiver
 * @given host
 * @when invalid receiver is passed
 * @then stream is reset and returned
 */
TEST_F(Libp2pDataTransferNetworkTest, SetInvalidDelegate) {
  std::shared_ptr<MessageReceiverMock> receiver = nullptr;
  EXPECT_CALL(*host, setProtocolHandler(Eq(kDataTransferLibp2pProtocol), _))
      .WillOnce(::testing::Invoke(
          this, &Libp2pDataTransferNetworkTest::captureProtocolHandler));
  EXPECT_OUTCOME_TRUE_1(network->setDelegate(receiver));

  std::shared_ptr<libp2p::connection::StreamMock> stream =
      std::make_shared<libp2p::connection::StreamMock>();
  EXPECT_CALL(*stream, reset()).WillOnce(testing::Return());
  protocol_handler_(stream);
}
