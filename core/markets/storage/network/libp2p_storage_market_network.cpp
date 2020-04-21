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

  outcome::result<std::shared_ptr<CborStream>>
  Libp2pStorageMarketNetwork::newAskStream(const PeerId &peer_id) {
    PeerInfo peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(
        peer_info,
        kAskProtocolId,
        [this, &stream](outcome::result<std::shared_ptr<Stream>> new_stream) {
          if (new_stream.has_error()) {
            logger_->error("cannot connect, msg='{}'",
                           new_stream.error().message());
          } else {
            stream = std::move(new_stream.value());
          }
        });
    return std::make_shared<CborStream>(stream);
  }

  outcome::result<std::shared_ptr<CborStream>>
  Libp2pStorageMarketNetwork::newDealStream(const PeerId &peer_id) {
    PeerInfo peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(
        peer_info,
        kDealProtocolId,
        [this, &stream](outcome::result<std::shared_ptr<Stream>> new_stream) {
          if (new_stream.has_error()) {
            logger_->error("cannot connect, msg='{}'",
                           new_stream.error().message());
          } else {
            stream = std::move(new_stream.value());
          }
        });
    return std::make_shared<CborStream>(stream);
  }

  outcome::result<void> Libp2pStorageMarketNetwork::setDelegate(
      std::shared_ptr<StorageReceiver> receiver) {
    receiver_ = std::move(receiver);
    host_->setProtocolHandler(
        kAskProtocolId, [this](std::shared_ptr<Stream> stream) {
          if (!receiver_) {
            logger_->error("Receiver is not set");
            stream->reset();
            return;
          }
          auto maybe_peer_id = stream->remotePeerId();
          if (maybe_peer_id.has_error()) {
            logger_->error("Cannot get remote peer id: "
                           + maybe_peer_id.error().message());
            stream->reset();
            return;
          }
          receiver_->handleAskStream(std::make_shared<CborStream>(stream));
        });
    host_->setProtocolHandler(
        kDealProtocolId, [this](std::shared_ptr<Stream> stream) {
          if (!receiver_) {
            logger_->error("Receiver is not set");
            stream->reset();
            return;
          }
          auto maybe_peer_id = stream->remotePeerId();
          if (maybe_peer_id.has_error()) {
            logger_->error("Cannot get remote peer id: "
                           + maybe_peer_id.error().message());
            stream->reset();
            return;
          }
          receiver_->handleDealStream(std::make_shared<CborStream>(stream));
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
