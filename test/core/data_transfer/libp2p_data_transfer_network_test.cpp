/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/libp2p_data_transfer_network.hpp"

#include <gtest/gtest.h>
#include <libp2p/basic/message_read_writer_error.hpp>
#include <libp2p/multi/multihash.hpp>
#include "testutil/mocks/data_transfer/message_receiver_mock.hpp"
#include "testutil/mocks/libp2p/host_mock.hpp"
#include "testutil/mocks/libp2p/stream_mock.hpp"
#include "testutil/outcome.hpp"

using fc::common::Buffer;
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
using libp2p::multi::Multihash;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;
using libp2p::peer::Protocol;
using ::testing::_;
using ::testing::Eq;

/**
 * Helper class for stream.read() inspection.
 */
class ReadHandlerInspector {
 public:
  /**
   * Sets value returned by capture call
   * @param out
   */
  void setReadResult(gsl::span<uint8_t> out) {
    buffer.reserve(out.size());
    std::copy(out.begin(), out.end(), std::back_inserter(buffer));
  }

  /**
   * Returns value set and captures the read handler
   */
  void captureReadHandler(gsl::span<uint8_t> out,
                          size_t bytes,
                          ReadCallbackFunc cb) {
    std::copy(buffer.begin(), buffer.end(), out.begin());
    read_handler_ = std::move(cb);
  }

  /**
   * Get captured handler
   * @return
   */
  ReadCallbackFunc getReadHandler() {
    return read_handler_;
  }

 private:
  std::vector<uint8_t> buffer;
  ReadCallbackFunc read_handler_;
};

class Libp2pDataTransferNetworkTest : public ::testing::Test {
 public:
  /**
   * Creates dummy PeerId
   */
  fc::outcome::result<PeerId> generatePeerId(int value) {
    Buffer kBuffer = Buffer(43, 1);
    EXPECT_OUTCOME_TRUE(hash,
                        Multihash::create(libp2p::multi::sha256, kBuffer));
    EXPECT_OUTCOME_TRUE(peer_id, PeerId::fromHash(hash));
    return peer_id;
  }

  /**
   * Imitates network.setDelegate(Protocol, Handler) call and captures handler
   * for further inspection
   */
  void captureProtocolHandler(const Protocol &,
                              const std::function<Stream::Handler> &handler) {
    protocol_handler_ = handler;
  }

  ReadHandlerInspector read_handler_inspector;

  /**
   * Protocol handler callback
   */
  std::function<Stream::Handler> protocol_handler_;

  /**
   * Read handler
   */
  ReadCallbackFunc read_handler_;

  std::shared_ptr<libp2p::HostMock> host = std::make_shared<libp2p::HostMock>();
  std::shared_ptr<DataTransferNetwork> network =
      std::make_shared<Libp2pDataTransferNetwork>(host);
};

/**
 * Successful connect to peer
 */
TEST_F(Libp2pDataTransferNetworkTest, Connect) {
  EXPECT_OUTCOME_TRUE(peer_id, generatePeerId(1));
  PeerInfo peer_info{.id = peer_id, .addresses = {}};
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

/**
 * @given host with receiver set
 * @when send request
 * @then receiver request handler is called
 */
TEST_F(Libp2pDataTransferNetworkTest, SendRequestMessage) {
  std::shared_ptr<MessageReceiverMock> receiver =
      std::make_shared<MessageReceiverMock>();
  EXPECT_CALL(*host, setProtocolHandler(Eq(kDataTransferLibp2pProtocol), _))
      .WillOnce(::testing::Invoke(
          this, &Libp2pDataTransferNetworkTest::captureProtocolHandler));
  EXPECT_OUTCOME_TRUE_1(network->setDelegate(receiver));

  // read request from stream
  std::string base_cid = "base_cid";
  bool is_pull = true;
  std::vector<uint8_t> selector{0x0, 0x1, 0x2, 0x3};
  std::vector<uint8_t> voucher{0x11, 0x12, 0x13, 0x14};
  std::string voucher_type = "fake_voucher";
  TransferId transfer_id = 1;

  DataTransferMessage request = createRequest(
      base_cid, is_pull, selector, voucher, voucher_type, transfer_id);
  EXPECT_OUTCOME_TRUE(request_bytes, fc::codec::cbor::encode(request));
  read_handler_inspector.setReadResult(request_bytes);

  std::shared_ptr<libp2p::connection::StreamMock> stream =
      std::make_shared<libp2p::connection::StreamMock>();
  EXPECT_CALL(*stream, read(_, _, _))
      .WillOnce(::testing::Invoke(&read_handler_inspector,
                                  &ReadHandlerInspector::captureReadHandler));

  EXPECT_OUTCOME_TRUE(peer_id, generatePeerId(2));
  EXPECT_CALL(*stream, remotePeerId())
      .WillOnce(testing::Return(fc::outcome::success(peer_id)));
  EXPECT_CALL(*receiver, receiveRequest(_, _))
      .WillOnce(testing::Return(fc::outcome::failure(
          libp2p::basic::MessageReadWriterError::BUFFER_IS_EMPTY)));
  // cancel stream by returning an error
  EXPECT_CALL(*receiver, receiveError()).WillOnce(testing::Return());
  EXPECT_CALL(*stream, reset()).WillOnce(testing::Return());

  protocol_handler_(stream);

  read_handler_inspector.getReadHandler()(fc::outcome::success());
}

/**
 * @given host with receiver set
 * @when send response
 * @then receiver response handler is called
 */
TEST_F(Libp2pDataTransferNetworkTest, SendResponseMessage) {
  std::shared_ptr<MessageReceiverMock> receiver =
      std::make_shared<MessageReceiverMock>();
  EXPECT_CALL(*host, setProtocolHandler(Eq(kDataTransferLibp2pProtocol), _))
      .WillOnce(::testing::Invoke(
          this, &Libp2pDataTransferNetworkTest::captureProtocolHandler));
  EXPECT_OUTCOME_TRUE_1(network->setDelegate(receiver));

  // read message from stream
  bool is_accepted = true;
  TransferId transfer_id = 1;

  DataTransferMessage message = createResponse(is_accepted, transfer_id);
  EXPECT_OUTCOME_TRUE(message_bytes, fc::codec::cbor::encode(message));
  read_handler_inspector.setReadResult(message_bytes);

  std::shared_ptr<libp2p::connection::StreamMock> stream =
      std::make_shared<libp2p::connection::StreamMock>();
  EXPECT_CALL(*stream, read(_, _, _))
      .WillOnce(::testing::Invoke(&read_handler_inspector,
                                  &ReadHandlerInspector::captureReadHandler));

  EXPECT_OUTCOME_TRUE(peer_id, generatePeerId(2));
  EXPECT_CALL(*stream, remotePeerId())
      .WillOnce(testing::Return(fc::outcome::success(peer_id)));
  EXPECT_CALL(*receiver, receiveResponse(_, _))
      .WillOnce(testing::Return(fc::outcome::failure(
          libp2p::basic::MessageReadWriterError::BUFFER_IS_EMPTY)));
  // cancel stream by returning an error
  EXPECT_CALL(*receiver, receiveError()).WillOnce(testing::Return());
  EXPECT_CALL(*stream, reset()).WillOnce(testing::Return());

  protocol_handler_(stream);

  read_handler_inspector.getReadHandler()(fc::outcome::success());
}
