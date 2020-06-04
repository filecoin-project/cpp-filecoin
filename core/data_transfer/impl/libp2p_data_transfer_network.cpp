/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/libp2p_data_transfer_network.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "data_transfer/message.hpp"

/**
 * Check outcome and calls receiver->receiveError in case of failure
 */
#define CHECK_OUTCOME_RESULT(expression, receiver) \
  if (expression.has_error()) {                    \
    receiver->receiveError();                      \
    stream->stream()->reset();                     \
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

  using common::libp2p::CborStream;
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
          auto cbor_stream = std::make_shared<CborStream>(stream);
          GET_OUTCOME_RESULT(peer_id, stream->remotePeerId(), this->receiver_);
          GET_OUTCOME_RESULT(
              multiaddress, stream->remoteMultiaddr(), this->receiver_);
          PeerInfo remote_peer_info{.id = peer_id, .addresses = {multiaddress}};
          while (true) {
            cbor_stream->read<DataTransferMessage>(
                [self{shared_from_this()},
                 remote_peer_info,
                 stream{cbor_stream}](
                    outcome::result<DataTransferMessage> message) {
                  if (message.has_error()) {
                    self->receiver_->receiveError();
                    return;
                  }
                  if (message.value().is_request) {
                    CHECK_OUTCOME_RESULT(
                        self->receiver_->receiveRequest(
                            remote_peer_info, *message.value().request),
                        self->receiver_);
                  } else {
                    CHECK_OUTCOME_RESULT(
                        self->receiver_->receiveResponse(
                            remote_peer_info, *message.value().response),
                        self->receiver_);
                  }
                });
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
