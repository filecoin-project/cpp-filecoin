/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_NETWORK_LIBP2P_STORAGE_MARKET_NETWORK_HPP
#define CPP_FILECOIN_MARKETS_NETWORK_LIBP2P_STORAGE_MARKET_NETWORK_HPP

#include <libp2p/host/host.hpp>
#include "common/logger.hpp"
#include "markets/network/market_network.hpp"

namespace fc::markets::network {
  using libp2p::Host;

  /**
   * A storage market network on top of libp2p.
   * Transforms the libp2p host interface, which sends and receives NetMessage
   * objects, into the graphsync network interface
   */
  class Libp2pMarketNetwork
      : public MarketNetwork,
        public std::enable_shared_from_this<Libp2pMarketNetwork> {
   public:
    explicit Libp2pMarketNetwork(std::shared_ptr<Host> host);

    auto newStream(const PeerInfo &peer,
                   const Protocol &protocol,
                   const CborStreamResultHandler &handler) -> void override;

    auto setDelegate(
        const Protocol &protocol,
        const std::function<libp2p::connection::Stream::Handler> &handler)
        -> outcome::result<void> override;

    /**
     * Closes stream and handles close result
     * @param stream to close
     */
    auto closeStreamGracefully(const std::shared_ptr<CborStream> &stream) const
        -> void override;

   private:
    std::shared_ptr<Host> host_;
    common::Logger logger_ = common::createLogger("Libp2pStorageMarketNetwork");
  };

}  // namespace fc::markets::network

#endif  // CPP_FILECOIN_MARKETS_NETWORK_LIBP2P_STORAGE_MARKET_NETWORK_HPP
