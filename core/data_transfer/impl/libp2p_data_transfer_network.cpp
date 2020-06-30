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
#define _CHECK_OUTCOME_RESULT(res, expression, receiver)         \
  auto &&res = (expression);                                     \
  if (res.has_error()) {                                         \
    self->logger_->error("Read error " + res.error().message()); \
    receiver->receiveError();                                    \
    stream->stream()->reset();                                   \
    return;                                                      \
  }
#define CHECK_OUTCOME_RESULT(expression, receiver) \
  _CHECK_OUTCOME_RESULT(UNIQUE_NAME(_r), expression, receiver)

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
        [self_weak{weak_from_this()}](
            std::shared_ptr<libp2p::connection::Stream> stream) {
          if (auto self = self_weak.lock()) {
            if (!self->receiver_) {
              stream->reset();
              return;
            }
            auto cbor_stream = std::make_shared<CborStream>(stream);
            GET_OUTCOME_RESULT(
                peer_id, stream->remotePeerId(), self->receiver_);
            GET_OUTCOME_RESULT(
                multiaddress, stream->remoteMultiaddr(), self->receiver_);
            PeerInfo remote_peer_info{.id = peer_id,
                                      .addresses = {multiaddress}};
            cbor_stream->read<DataTransferMessage>(
                [self_weak, remote_peer_info, stream{cbor_stream}](
                    outcome::result<DataTransferMessage> message) {
                  if (auto self = self_weak.lock()) {
                    self->logger_->debug("New message");
                    CHECK_OUTCOME_RESULT(message, self->receiver_);
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

  void Libp2pDataTransferNetwork::sendMessage(
      const PeerInfo &to, const DataTransferMessage &message) {
    host_->newStream(
        to,
        kDataTransferLibp2pProtocol,
        [self{shared_from_this()},
         message](outcome::result<std::shared_ptr<libp2p::connection::Stream>>
                      stream_res) {
          if (stream_res.has_error()) {
            self->logger_->error("Open stream error "
                                 + stream_res.error().message());
            return;
          }
          CborStream stream(stream_res.value());
          stream.write(
              message, [self, stream](outcome::result<size_t> written) {
                if (written.has_error()) {
                  self->logger_->error("Send error "
                                       + written.error().message());
                  return;
                }
                self->logger_->debug("Message sent "
                                     + std::to_string(written.value()));
                // TODO (a.chernyshov) read response
                // validate response voucher
              });
        });
  }

}  // namespace fc::data_transfer
