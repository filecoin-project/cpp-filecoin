/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/network/libp2p_ask_stream.hpp"
#include "markets/storage/network/libp2p_deal_stream.hpp"

namespace fc::markets::storage::network {

  using libp2p::connection::Stream;
  using libp2p::peer::PeerInfo;

  outcome::result<std::shared_ptr<StorageAskStream>>
  Libp2pStorageMarketNetwork::newAskStream(const PeerId &peer_id) {
    PeerInfo peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(
        peer_info,
        kAskProtocolId,
        [&stream](outcome::result<std::shared_ptr<Stream>> new_stream) {
          stream = std::move(new_stream.value());
        });
    return std::make_shared<Libp2pAskStream>(peer_id, stream);
  }

  outcome::result<std::shared_ptr<StorageDealStream>>
  Libp2pStorageMarketNetwork::newDealStream(const PeerId &peer_id) {
    PeerInfo peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    std::shared_ptr<libp2p::connection::Stream> stream;
    host_->newStream(
        peer_info,
        kAskProtocolId,
        [&stream](outcome::result<std::shared_ptr<Stream>> new_stream) {
          stream = std::move(new_stream.value());
        });
    return std::make_shared<Libp2pDealStream>(peer_id, stream);
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
          receiver_->handleAskStream(
              std::make_shared<Libp2pAskStream>(maybe_peer_id.value(), stream));
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
          receiver_->handleDealStream(std::make_shared<Libp2pDealStream>(
              maybe_peer_id.value(), stream));
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
