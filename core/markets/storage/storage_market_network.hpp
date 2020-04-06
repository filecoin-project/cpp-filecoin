/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_MARKET_NETWORK_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_MARKET_NETWORK_HPP

#include <libp2p/peer/peer_id.hpp>
#include "common/outcome.hpp"
#include "markets/storage/ask_stream.hpp"
#include "markets/storage/deal_stream.hpp"
#include "markets/storage/storage_receiver.hpp"

namespace fc::markets::storage {

  using libp2p::peer::PeerId;

  /**
   * StorageMarketNetwork is a network abstraction for the storage market
   */
  class StorageMarketNetwork {
   public:
    virtual ~StorageMarketNetwork() = 0;

    virtual auto newAskStream(const PeerId &peer)
        -> outcome::result<std::shared_ptr<StorageAskStream>> = 0;

    virtual auto newDealStream(const PeerId &peer)
        -> outcome::result<std::shared_ptr<StorageDealStream>> = 0;

    virtual auto setDelegate(std::shared_ptr<StorageReceiver> receiver)
        -> outcome::result<void> = 0;

    virtual auto stopHandlingRequests() -> outcome::result<void> = 0;
  };
}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_MARKET_NETWORK_HPP
