/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/libp2p_data_transfer_network.hpp"
#include "data_transfer/message.hpp"

/**
 * Check outcome and calls receiver->receiveError in case of failure
 */
#define CHECK_OUTCOME_RESULT(expression, receiver) \
  if (expression.has_error()) {                    \
    receiver->receiveError();                      \
    stream->reset();                               \
    return;                                        \
  }

#define GET_OUTCOME_RESULT(result, expression, receiver) \
  auto UNIQUE_NAME(_r) = expression;                     \
  if (UNIQUE_NAME(_r).has_error()) {                     \
    receiver->receiveError();                            \
    stream->reset();                                     \
    return;                                              \
  }                                                      \
  auto result = UNIQUE_NAME(_r).value();

namespace fc::data_transfer {

  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;

  Libp2pDataTransferNetwork::Libp2pDataTransferNetwork(
      std::shared_ptr<libp2p::Host> host)
      : host_(std::move(host)) {}

  outcome::result<void> Libp2pDataTransferNetwork::setDelegate(
      std::shared_ptr<MessageReceiver> receiver) {
    receiver_ = std::move(receiver);
    host_->setProtocolHandler(
        kDataTransferLibp2pProtocol,
        [this](std::shared_ptr<libp2p::connection::Stream> stream) {
          if (!this->receiver_) {
            stream->reset();
            return;
          }
          while (true) {
            // read up to buffer_max bytes and try decode
            std::array<uint8_t, 1024> buffer{0};
            stream->read(
                buffer, buffer.size(), [this](outcome::result<size_t> read) {
                  if (!read) {
                    this->receiver_->receiveError();
                  }
                });
            GET_OUTCOME_RESULT(message,
                               codec::cbor::decode<DataTransferMessage>(buffer),
                               this->receiver_);
            GET_OUTCOME_RESULT(
                peer_id, stream->remotePeerId(), this->receiver_);
            GET_OUTCOME_RESULT(
                multiaddress, stream->remoteMultiaddr(), this->receiver_);
            PeerInfo remote_peer_info{.id = peer_id,
                                      .addresses = {multiaddress}};

            if (message.is_request) {
              CHECK_OUTCOME_RESULT(
                  receiver_->receiveRequest(remote_peer_info, *message.request),
                  this->receiver_);
            } else {
              CHECK_OUTCOME_RESULT(receiver_->receiveResponse(
                                       remote_peer_info, *message.response),
                                   this->receiver_);
            }
          }
        });
    return outcome::success();
  }

  outcome::result<void> Libp2pDataTransferNetwork::connectTo(
      const PeerInfo &peer_info) {
    host_->connect(peer_info);
    return outcome::success();
  }

  outcome::result<std::shared_ptr<MessageSender>>
  Libp2pDataTransferNetwork::newMessageSender(const PeerInfo &peer_info) {
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(
        peer_info,
        kDataTransferLibp2pProtocol,
        [&stream](
            outcome::result<std::shared_ptr<libp2p::connection::Stream>> s) {
          stream = std::move(s.value());
        });
    return std::make_shared<StreamMessageSender>(stream);
  }

}  // namespace fc::data_transfer
