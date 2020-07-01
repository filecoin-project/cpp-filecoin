/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_NETWORK_MARKET_NETWORK_HPP
#define CPP_FILECOIN_CORE_MARKETS_NETWORK_MARKET_NETWORK_HPP

#include <libp2p/peer/peer_info.hpp>
#include "common/libp2p/cbor_stream.hpp"
#include "common/outcome.hpp"

namespace fc::markets::network {
  using common::libp2p::CborStream;
  using libp2p::peer::PeerInfo;
  using libp2p::peer::Protocol;
  using CborStreamResultHandler =
      std::function<void(outcome::result<std::shared_ptr<CborStream>>)>;
  using NewStreamHandler = std::function<libp2p::connection::Stream::Handler>;

  /**
   * MarketNetwork is a network abstraction for the storage market
   */
  class MarketNetwork {
   public:
    virtual ~MarketNetwork() = default;

    /**
     * Creates new CBOR stream of a given protocol
     * @param peer connect to
     * @param protocol of a stream
     * @param handler - new stream handler
     */
    virtual auto newStream(const PeerInfo &peer,
                           const Protocol &protocol,
                           const CborStreamResultHandler &handler) -> void = 0;

    /**
     * Set income stream handler for protocol
     * @param protocol of a stream
     * @param handler - incoming stream handler
     * @return nothing on success or error in case of failure
     */
    virtual auto setDelegate(const Protocol &protocol,
                             const NewStreamHandler &handler)
        -> outcome::result<void> = 0;

    /**
     * Closes stream and handles close result
     * @param stream to close
     */
    virtual auto closeStreamGracefully(
        const std::shared_ptr<CborStream> &stream) const -> void = 0;
  };
}  // namespace fc::markets::network

#endif  // CPP_FILECOIN_CORE_MARKETS_NETWORK_MARKET_NETWORK_HPP
