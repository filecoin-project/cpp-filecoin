/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/network/impl/libp2p_market_network.hpp"

namespace fc::markets::network {
  using libp2p::connection::Stream;
  using libp2p::peer::PeerInfo;

  Libp2pMarketNetwork::Libp2pMarketNetwork(
      std::shared_ptr<Host> host)
      : host_{std::move(host)} {}

  void Libp2pMarketNetwork::newStream(
      const PeerInfo &peer_info,
      const Protocol &protocol,
      const CborStreamResultHandler &handler) {
    host_->newStream(peer_info, protocol, [handler](auto stream) {
      if (stream.has_error()) {
        handler(stream.error());
        return;
      }
      handler(std::make_shared<CborStream>(stream.value()));
    });
  }

  outcome::result<void> Libp2pMarketNetwork::setDelegate(
      const Protocol &protocol,
      const std::function<libp2p::connection::Stream::Handler> &handler) {
    host_->setProtocolHandler(protocol, handler);
    return outcome::success();
  }

  outcome::result<void> Libp2pMarketNetwork::stopHandlingRequests() {
    // TODO (a.chernyshov) remove protocol handlers - not supported by
    // cpp-libp2p Host
    return outcome::success();
  }

  void Libp2pMarketNetwork::closeStreamGracefully(
      const std::shared_ptr<CborStream> &stream) const {
    if (!stream->stream()->isClosed()) {
      stream->stream()->close([logger{logger_}](
                                  outcome::result<void> close_res) {
        logger->debug("Close stream");
        if (close_res.has_error()) {
          logger->error("Close stream error " + close_res.error().message());
        }
      });
    }
  }

}  // namespace fc::markets::storage::network
