/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_NETWORK_LIBP2P_STORAGE_MARKET_NETWORK_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_NETWORK_LIBP2P_STORAGE_MARKET_NETWORK_HPP

#include <libp2p/host/host.hpp>
#include "common/logger.hpp"
#include "markets/storage/storage_market_network.hpp"
#include "markets/storage/storage_receiver.hpp"

namespace fc::markets::storage::network {

  using libp2p::Host;

  /**
   * A storage market network on top of libp2p.
   * Libp2pStorageMarketNetwork transforms the libp2p host interface, which
   * sends and receives NetMessage objects, into the graphsync network interface
   */
  class Libp2pStorageMarketNetwork
      : public StorageMarketNetwork,
        public std::enable_shared_from_this<Libp2pStorageMarketNetwork> {
   public:
    auto newAskStream(const PeerId &peer_id,
                      const CborStreamResultHandler &handler) -> void override;

    auto newDealStream(const PeerId &peer_id,
                       const CborStreamResultHandler &handler) -> void override;

    auto setDelegate(std::shared_ptr<StorageReceiver> receiver)
        -> outcome::result<void> override;

    auto stopHandlingRequests() -> outcome::result<void> override;

   private:
    std::shared_ptr<Host> host_;

    /** inbound messages from the network are forwarded to the receiver */
    std::shared_ptr<StorageReceiver> receiver_;

    common::Logger logger_ = common::createLogger("Libp2pStorageMarketNetwork");
  };

}  // namespace fc::markets::storage::network

#endif  // CPP_FILECOIN_MARKETS_STORAGE_NETWORK_LIBP2P_STORAGE_MARKET_NETWORK_HPP
