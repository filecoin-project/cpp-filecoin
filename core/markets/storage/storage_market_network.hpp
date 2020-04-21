/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_MARKET_NETWORK_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_MARKET_NETWORK_HPP

#include <libp2p/peer/peer_id.hpp>
#include "common/libp2p/cbor_stream.hpp"
#include "common/outcome.hpp"
#include "markets/storage/storage_receiver.hpp"

namespace fc::markets::storage {

  using common::libp2p::CborStream;
  using libp2p::peer::PeerId;
  using StreamResultHandler = libp2p::Host::StreamResultHandler;

  /**
   * StorageMarketNetwork is a network abstraction for the storage market
   */
  class StorageMarketNetwork {
   public:
    virtual ~StorageMarketNetwork() = 0;

    virtual auto newAskStream(const PeerId &peer,
                              const StreamResultHandler &handler)
        -> outcome::result<std::shared_ptr<CborStream>> = 0;

    virtual auto newDealStream(const PeerId &peer,
                               const StreamResultHandler &handler)
        -> outcome::result<std::shared_ptr<CborStream>> = 0;

    virtual auto setDelegate(std::shared_ptr<StorageReceiver> receiver)
        -> outcome::result<void> = 0;

    virtual auto stopHandlingRequests() -> outcome::result<void> = 0;
  };
}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_MARKET_NETWORK_HPP
