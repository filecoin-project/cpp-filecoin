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
  if (expression.has_error()) {                          \
    receiver->receiveError();                            \
    stream->reset();                                     \
    return;                                              \
  }                                                      \
  auto result = expression.value();

namespace fc::data_transfer {

  using libp2p::peer::PeerId;

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
            auto maybe_peer_id = stream->remotePeerId();
            if (maybe_peer_id.has_error()) {
              receiver_->receiveError();
              stream->reset();
              return;
            }
            PeerInfo peer_info{.id = maybe_peer_id.value(), .addresses = {}};

            if (message.is_request) {
              CHECK_OUTCOME_RESULT(
                  receiver_->receiveRequest(peer_info, *message.request),
                  this->receiver_);
            } else {
              CHECK_OUTCOME_RESULT(
                  receiver_->receiveResponse(peer_info, *message.response),
                  this->receiver_);
            }
          }
        });
    return outcome::success();
  }

  outcome::result<void> Libp2pDataTransferNetwork::connectTo(
      const PeerInfo &peer) {
    host_->connect(peer);
    return outcome::success();
  }

  outcome::result<std::shared_ptr<MessageSender>>
  Libp2pDataTransferNetwork::newMessageSender(const PeerInfo &peer) {
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(
        peer,
        kDataTransferLibp2pProtocol,
        [&stream](
            outcome::result<std::shared_ptr<libp2p::connection::Stream>> s) {
          stream = std::move(s.value());
        });
    return std::make_shared<StreamMessageSender>(stream);
  }

}  // namespace fc::data_transfer
