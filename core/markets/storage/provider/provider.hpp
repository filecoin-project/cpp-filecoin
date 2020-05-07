/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP

#include <libp2p/host/host.hpp>
#include "common/logger.hpp"
#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "markets/storage/provider.hpp"
#include "markets/storage/storage_receiver.hpp"
#include "stored_ask.hpp"

namespace fc::markets::storage::provider {

  using libp2p::Host;
  using network::Libp2pStorageMarketNetwork;

  class StorageProviderImpl
      : public StorageProvider,
        public StorageReceiver,
        public std::enable_shared_from_this<StorageProviderImpl> {
   public:
    StorageProviderImpl(std::shared_ptr<Host> host,
                        std::shared_ptr<boost::asio::io_context> context,
                        std::shared_ptr<KeyStore> keystore,
                        std::shared_ptr<Datastore> datastore,
                        std::shared_ptr<Api> api,
                        const Address &actor_address);

    auto start() -> outcome::result<void> override;

    auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void> override;

    auto listAsks(const Address &address)
        -> outcome::result<std::vector<SignedStorageAsk>> override;

    auto listDeals() -> outcome::result<std::vector<StorageDeal>> override;

    auto listIncompleteDeals()
        -> outcome::result<std::vector<MinerDeal>> override;

    auto addStorageCollateral(const TokenAmount &amount)
        -> outcome::result<void> override;

    auto getStorageCollateral() -> outcome::result<TokenAmount> override;

    auto importDataForDeal(const CID &prop_cid,
                           const libp2p::connection::Stream &data)
        -> outcome::result<void> override;

   protected:
    auto handleAskStream(const std::shared_ptr<CborStream> &stream)
        -> void override;

    auto handleDealStream(const std::shared_ptr<CborStream> &stream)
        -> void override;

   private:
    /**
     * If error is present, closes connection and prints message
     * @tparam T - result type
     * @param res - result to check for error
     * @param on_error_msg - message to log on error
     * @param stream - stream to close on error
     * @return true if
     */
    template <class T>
    bool hasValue(outcome::result<T> res,
                  const std::string &on_error_msg,
                  const std::shared_ptr<CborStream> &stream) const {
      if (res.has_error()) {
        logger_->error(on_error_msg + res.error().message());
        network_->closeStreamGracefully(stream);
        return false;
      }
      return true;
    };

    /**
     * Closes stream and handles close result
     * @param stream to close
     */
    void closeStreamGracefully(const std::shared_ptr<CborStream> &stream) const;

    std::shared_ptr<Host> host_;
    std::shared_ptr<boost::asio::io_context> context_;

    std::shared_ptr<StoredAsk> stored_ask_;

    std::shared_ptr<StorageMarketNetwork> network_;

    common::Logger logger_ = common::createLogger("StorageMarketProvider");
  };

}  // namespace fc::markets::storage::provider

#endif  // CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
