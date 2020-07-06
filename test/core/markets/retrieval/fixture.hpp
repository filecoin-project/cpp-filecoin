/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/client/impl/retrieval_client_impl.hpp"
#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"

#include <gtest/gtest.h>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/security/plaintext.hpp>
#include "api/api.hpp"
#include "core/markets/retrieval/config.hpp"
#include "core/markets/retrieval/data.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"

namespace fc::markets::retrieval::test {
  using primitives::tipset::Tipset;

  struct RetrievalMarketFixture : public ::testing::Test {
    /* Types */
    using ClientShPtr = std::shared_ptr<client::RetrievalClientImpl>;
    using ProviderShPtr = std::shared_ptr<provider::RetrievalProviderImpl>;
    using ContextShPtr = std::shared_ptr<boost::asio::io_context>;
    using HostShPtr = std::shared_ptr<libp2p::Host>;
    using PieceStorageShPtr =
        std::shared_ptr<::fc::storage::piece::PieceStorage>;
    using StorageShPtr = std::shared_ptr<::fc::storage::InMemoryStorage>;
    using Multiaddress = libp2p::multi::Multiaddress;
    using ApiShPtr = std::shared_ptr<api::Api>;

    /* Retrieval market client */
    ClientShPtr client;

    /* Retrieval market provider */
    ProviderShPtr provider;

    /* Libp2p network host */
    HostShPtr host;

    /* I/O context */
    ContextShPtr context;

    /* Piece storage */
    PieceStorageShPtr piece_storage;

    /* Common storage backend */
    StorageShPtr storage_backend;

    /* Common application interface */
    ApiShPtr api;

    common::Logger logger = common::createLogger("RetrievalMarketTest");

    /**
     * @brief Constructor
     */
    RetrievalMarketFixture() {
      auto injector = libp2p::injector::makeHostInjector(
          libp2p::injector::useKeyPair(config::provider::keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
      host = injector.create<std::shared_ptr<libp2p::Host>>();
      context = injector.create<std::shared_ptr<boost::asio::io_context>>();
      storage_backend = std::make_shared<::fc::storage::InMemoryStorage>();
      api = std::make_shared<api::Api>();
      this->piece_storage =
          std::make_shared<::fc::storage::piece::PieceStorageImpl>(
              storage_backend);
      provider = std::make_shared<provider::RetrievalProviderImpl>(
          host, api, piece_storage);
      client = std::make_shared<client::RetrievalClientImpl>(host);
      provider->start();
    }

    /**
     * @brief On before all test cases
     */
    void SetUp() override {
      auto listen_address =
          Multiaddress::create(config::provider::multiaddress);
      BOOST_ASSERT_MSG(listen_address.has_value(),
                       "Failed to create server multiaddress");
      auto listen_status = host->listen(listen_address.value());
      BOOST_ASSERT_MSG(listen_status.has_value(),
                       "Failed to listen on provider multiaddress");
      host->start();
      std::thread([this]() { context->run(); }).detach();
      BOOST_ASSERT_MSG(addPieceSample(data::green_piece).has_value(),
                       "Failed to add sample green piece");

      Tipset chain_head;
      Address miner_worker_address = Address::makeFromId(100);
      api->ChainHead = {[=]() { return chain_head; }};

      api->StateMinerWorker = {
          [=](auto &address, auto &tipset_key) -> outcome::result<Address> {
            return miner_worker_address;
          }};
    }

    /**
     * @brief On all test finished
     */
    void TearDown() override {
      context->stop();
    }

    outcome::result<void> addPieceSample(const SamplePiece &piece) {
      OUTCOME_TRY(piece_storage->addPieceInfo(piece.cid, piece.info));
      for (const auto &payload : piece.payloads) {
        OUTCOME_TRY(piece_storage->addPayloadLocations(
            piece.cid, {{payload.cid, payload.location}}));
      }
      return outcome::success();
    }
  };
}  // namespace fc::markets::retrieval::test
