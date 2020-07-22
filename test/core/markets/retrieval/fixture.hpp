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
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"

namespace fc::markets::retrieval::test {
  using api::AddChannelInfo;
  using common::Buffer;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::piece::DealInfo;
  using fc::storage::piece::PayloadLocation;
  using primitives::tipset::Tipset;
  using provider::ProviderConfig;
  using vm::actor::builtin::payment_channel::SignedVoucher;

  /** Shared resources */
  static std::shared_ptr<libp2p::Host> host;
  static std::shared_ptr<boost::asio::io_context> context;

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

    static void SetUpTestCase() {
      auto injector = libp2p::injector::makeHostInjector(
          libp2p::injector::useKeyPair(config::provider::keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
      host = injector.create<std::shared_ptr<libp2p::Host>>();
      context = injector.create<std::shared_ptr<boost::asio::io_context>>();
      auto listen_address =
          Multiaddress::create(config::provider::multiaddress);
      BOOST_ASSERT_MSG(listen_address.has_value(),
                       "Failed to create server multiaddress");
      auto listen_status = host->listen(listen_address.value());
      BOOST_ASSERT_MSG(listen_status.has_value(),
                       "Failed to listen on provider multiaddress");
      host->start();
      std::thread([]() { context->run(); }).detach();
    }

    static void TearDownTestCase() {
      host.reset();
      context.reset();
    }

    /* Retrieval market client */
    ClientShPtr client;

    /* Retrieval market provider */
    ProviderShPtr provider;

    /* Piece storage */
    PieceStorageShPtr piece_storage;

    /* Common storage backend */
    StorageShPtr storage_backend;

    /* Common application interface */
    ApiShPtr api;

    /** IPFS datastore */
    std::shared_ptr<IpfsDatastore> client_ipfs{
        std::make_shared<InMemoryDatastore>()};
    std::shared_ptr<IpfsDatastore> provider_ipfs{
        std::make_shared<InMemoryDatastore>()};

    /** filecoin addresses */
    Address miner_worker_address = Address::makeFromId(100);
    Address miner_wallet = Address::makeFromId(101);
    Address client_wallet = Address::makeFromId(200);
    CID payload_cid;

    common::Logger logger = common::createLogger("RetrievalMarketTest");

    /**
     * @brief Constructor
     */
    RetrievalMarketFixture() {
      storage_backend = std::make_shared<::fc::storage::InMemoryStorage>();
      api = std::make_shared<api::Api>();
      piece_storage = std::make_shared<::fc::storage::piece::PieceStorageImpl>(
          storage_backend);
      ProviderConfig config{.price_per_byte = 2,
                            .payment_interval = 100,
                            .interval_increase = 10};
      provider = std::make_shared<provider::RetrievalProviderImpl>(
          host, api, piece_storage, provider_ipfs, config);
      client =
          std::make_shared<client::RetrievalClientImpl>(host, api, client_ipfs);
      provider->start();
    }

    /**
     * @brief On before all test cases
     */
    void SetUp() override {
      BOOST_ASSERT_MSG(
          addPieceSample(data::green_piece, provider_ipfs).has_value(),
          "Failed to add sample green piece");

      Tipset chain_head;
      api->ChainHead = {[=]() { return chain_head; }};

      api->StateMinerWorker = {
          [=](auto &address, auto &tipset_key) -> outcome::result<Address> {
            return miner_worker_address;
          }};

      api->PaychGet = {
          [=](auto &, auto &, auto &) -> outcome::result<AddChannelInfo> {
            return AddChannelInfo{.channel = Address::makeFromId(333)};
          }};

      api->PaychAllocateLane = {
          [=](auto &) -> outcome::result<LaneId> { return LaneId{1}; }};

      api->PaychVoucherCreate = {
          [=](const Address &,
              const TokenAmount &,
              const LaneId &) -> outcome::result<SignedVoucher> {
            SignedVoucher voucher;
            voucher.amount = 100;
            return voucher;
          }};

      api->PaychVoucherAdd = {
          [=](const Address &,
              const SignedVoucher &,
              const Buffer &,
              const TokenAmount &) -> outcome::result<TokenAmount> {
            return TokenAmount{0};
          }};
    };

    outcome::result<void> addPieceSample(
        const SamplePiece &piece, const std::shared_ptr<IpfsDatastore> &ipfs) {
      Buffer payload{"deadface"_unhex};
      OUTCOME_TRY(bytes, codec::cbor::encode(payload));
      OUTCOME_TRYA(payload_cid, common::getCidOf(bytes));
      CID piece_cid =
          "12209139839e65fabea9efd230898ad8b574509147e48d7c1e87a33d6da70fd2efae"_cid;
      DealInfo deal{.deal_id = 18, .sector_id = 4, .offset = 128, .length = 64};
      PayloadLocation location{.relative_offset = 16, .block_size = 4};
      OUTCOME_TRY(piece_storage->addDealForPiece(piece_cid, deal));
      OUTCOME_TRY(piece_storage->addPayloadLocations(
          piece_cid, {{payload_cid, location}}));
      OUTCOME_TRY(ipfs->setCbor(payload));
      return outcome::success();
    }
  };
}  // namespace fc::markets::retrieval::test
