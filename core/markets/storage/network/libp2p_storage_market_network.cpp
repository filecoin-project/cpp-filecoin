/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"

namespace fc::markets::storage::network {

  using libp2p::connection::Stream;
  using libp2p::peer::PeerInfo;

  void Libp2pStorageMarketNetwork::newAskStream(
      const PeerId &peer_id, const CborStreamResultHandler &handler) {
    PeerInfo peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(peer_info, kAskProtocolId, [&handler](auto stream) {
      if (stream.has_error()) {
        handler(stream.error());
      }
      handler(std::make_shared<CborStream>(stream.value()));
    });
  }

  void Libp2pStorageMarketNetwork::newDealStream(
      const PeerId &peer_id, const CborStreamResultHandler &handler) {
    PeerInfo peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(peer_info, kDealProtocolId, [&handler](auto stream) {
      if (stream.has_error()) {
        handler(stream.error());
      }
      handler(std::make_shared<CborStream>(stream.value()));
    });
  }

  outcome::result<void> Libp2pStorageMarketNetwork::setDelegate(
      std::shared_ptr<StorageReceiver> receiver) {
    receiver_ = std::move(receiver);
    host_->setProtocolHandler(
        kAskProtocolId,
        [self{shared_from_this()}](std::shared_ptr<Stream> stream) {
          if (!self->receiver_) {
            self->logger_->error("Receiver is not set");
            stream->reset();
            return;
          }
          auto maybe_peer_id = stream->remotePeerId();
          if (maybe_peer_id.has_error()) {
            self->logger_->error("Cannot get remote peer id: "
                                 + maybe_peer_id.error().message());
            stream->reset();
            return;
          }
          self->receiver_->handleAskStream(
              std::make_shared<CborStream>(stream));
        });
    host_->setProtocolHandler(
        kDealProtocolId,
        [self{shared_from_this()}](std::shared_ptr<Stream> stream) {
          if (!self->receiver_) {
            self->logger_->error("Receiver is not set");
            stream->reset();
            return;
          }
          auto maybe_peer_id = stream->remotePeerId();
          if (maybe_peer_id.has_error()) {
            self->logger_->error("Cannot get remote peer id: "
                                 + maybe_peer_id.error().message());
            stream->reset();
            return;
          }
          self->receiver_->handleDealStream(
              std::make_shared<CborStream>(stream));
        });
    return outcome::success();
  }

  outcome::result<void> Libp2pStorageMarketNetwork::stopHandlingRequests() {
    receiver_.reset();
    // TODO (a.chernyshov) remove protocol handlers - not supported by
    // cpp-libp2p Host
    return outcome::success();
  }

}  // namespace fc::markets::storage::network
