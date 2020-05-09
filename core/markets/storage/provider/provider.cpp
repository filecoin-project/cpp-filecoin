/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/provider.hpp"

#include "common/todo_error.hpp"
#include "markets/storage/provider/storage_provider_error.hpp"
#include "markets/storage/provider/stored_ask.hpp"

namespace fc::markets::storage::provider {

  StorageProviderImpl::StorageProviderImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<KeyStore> keystore,
      std::shared_ptr<Datastore> datastore,
      std::shared_ptr<Api> api,
      const Address &actor_address)
      : host_{std::move(host)},
        context_{std::move(context)},
        stored_ask_{std::make_shared<StoredAsk>(
            keystore, datastore, api, actor_address)},
        network_{std::make_shared<Libp2pStorageMarketNetwork>(host_)} {}

  outcome::result<void> StorageProviderImpl::start() {
    OUTCOME_TRY(network_->setDelegate(shared_from_this()));

    context_->post([self{shared_from_this()}] {
      self->host_->start();
      self->logger_->debug(
          "Server started\nListening on: "
          + std::string(self->host_->getAddresses().front().getStringAddress())
          + "\nPeer id: " + self->host_->getPeerInfo().id.toBase58());
    });

    return outcome::success();
  }

  outcome::result<void> StorageProviderImpl::addAsk(const TokenAmount &price,
                                                    ChainEpoch duration) {
    return stored_ask_->addAsk(price, duration);
  }

  outcome::result<std::vector<SignedStorageAsk>> StorageProviderImpl::listAsks(
      const Address &address) {
    std::vector<SignedStorageAsk> result;
    OUTCOME_TRY(signed_storage_ask, stored_ask_->getAsk(address));
    result.push_back(signed_storage_ask);
    return result;
  }

  outcome::result<std::vector<StorageDeal>> StorageProviderImpl::listDeals() {
    return TodoError::ERROR;
  }

  outcome::result<std::vector<MinerDeal>>
  StorageProviderImpl::listIncompleteDeals() {
    return TodoError::ERROR;
  }

  outcome::result<void> StorageProviderImpl::addStorageCollateral(
      const TokenAmount &amount) {
    return TodoError::ERROR;
  }

  outcome::result<TokenAmount> StorageProviderImpl::getStorageCollateral() {
    return outcome::failure(TodoError::ERROR);
  }

  outcome::result<void> StorageProviderImpl::importDataForDeal(
      const CID &prop_cid, const libp2p::connection::Stream &data) {
    return TodoError::ERROR;
  }

  void StorageProviderImpl::handleAskStream(
      const std::shared_ptr<CborStream> &stream) {
    logger_->debug("New ask stream");
    stream->read<AskRequest>([self{shared_from_this()},
                              stream](outcome::result<AskRequest> request_res) {
      if (!self->hasValue(request_res, "Ask request error ", stream)) return;
      auto maybe_ask = self->stored_ask_->getAsk(request_res.value().miner);
      if (!self->hasValue(maybe_ask, "Get stored ask error ", stream)) return;
      AskResponse response{.ask = maybe_ask.value()};
      stream->write(
          response, [self, stream](outcome::result<size_t> maybe_res) {
            if (!self->hasValue(maybe_res, "Write ask response error ", stream))
              return;
            self->network_->closeStreamGracefully(stream);
            self->logger_->debug("Ask response written, connection closed");
          });
    });
  }

  void StorageProviderImpl::handleDealStream(
      const std::shared_ptr<CborStream> &stream) {
    logger_->debug("New deal stream");
  }

}  // namespace fc::markets::storage::provider
