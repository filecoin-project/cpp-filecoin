/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/client/impl/retrieval_client_impl.hpp"
#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"

#include <gtest/gtest.h>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/security/plaintext.hpp>

#include "api/full_node/node_api.hpp"
#include "common/libp2p/soralog.hpp"
#include "core/markets/retrieval/config.hpp"
#include "core/markets/retrieval/data.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/car/car.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"
#include "testutil/mocks/miner/miner_mock.hpp"
#include "testutil/mocks/sector_storage/manager_mock.hpp"

namespace fc::markets::retrieval::test {
  using api::AddChannelInfo;
  using api::MinerInfo;
  using data_transfer::DataTransfer;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::piece::DealInfo;
  using fc::storage::piece::PayloadLocation;
  using markets::retrieval::RetrievalAsk;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  static auto port{40010};

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
    using ApiShPtr = std::shared_ptr<api::FullNodeApi>;
    using MinerMockShPtr = std::shared_ptr<miner::MinerMock>;
    using ManagerMockShPtr = std::shared_ptr<sector_storage::ManagerMock>;

    std::shared_ptr<libp2p::Host> host;
    std::shared_ptr<boost::asio::io_context> context;
    std::shared_ptr<DataTransfer> datatransfer;

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

    ManagerMockShPtr sealer;

    MinerMockShPtr miner;

    DealInfo deal;

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

    void SetUp() override {
      libp2pSoralog();

      std::string address_string = fmt::format(
          "/ip4/127.0.0.1/tcp/{}/ipfs/"
          "12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV",
          port++);
      auto injector = libp2p::injector::makeHostInjector<
          boost::di::extension::shared_config>(
          libp2p::injector::useKeyPair(config::provider::keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
      host = injector.create<std::shared_ptr<libp2p::Host>>();
      context = injector.create<std::shared_ptr<boost::asio::io_context>>();
      OUTCOME_EXCEPT(listen_address, Multiaddress::create(address_string));
      OUTCOME_EXCEPT(host->listen(listen_address));
      host->start();

      storage_backend = std::make_shared<::fc::storage::InMemoryStorage>();
      api = std::make_shared<api::FullNodeApi>();
      piece_storage = std::make_shared<::fc::storage::piece::PieceStorageImpl>(
          storage_backend);
      RetrievalAsk config{
          .price_per_byte = 2,
          .unseal_price = 0,
          .payment_interval = 100,
          .interval_increase = 10,
      };

      sealer = std::make_shared<sector_storage::ManagerMock>();

      miner = std::make_shared<miner::MinerMock>();

      auto graphsync{
          std::make_shared<fc::storage::ipfs::graphsync::GraphsyncImpl>(
              host,
              injector.create<std::shared_ptr<libp2p::basic::Scheduler>>())};
      graphsync->subscribe([this](auto &from, auto &data) {
        OUTCOME_EXCEPT(client_ipfs->set(data.cid, data.content));
      });
      graphsync->start();
      datatransfer = DataTransfer::make(host, graphsync);

      auto config_key{
          std::make_shared<fc::storage::OneKey>("config", storage_backend)};
      config_key->setCbor(config);
      provider = std::make_shared<provider::RetrievalProviderImpl>(
          host, datatransfer, api, piece_storage, config_key, sealer, miner);
      client = std::make_shared<client::RetrievalClientImpl>(
          host, datatransfer, api, client_ipfs);
      provider->start();

      addPieceSample(data::green_piece, provider_ipfs).value();

      TipsetCPtr chain_head = std::make_shared<Tipset>();
      api->ChainHead = {[=]() { return chain_head; }};

      api->StateMinerInfo = [=](auto &address, auto &tipset_key) {
        MinerInfo info;
        info.worker = miner_worker_address;
        info.multiaddrs = host->getAddresses();
        return info;
      };

      api->PaychGet = [=](auto cb, auto &, auto &, auto &) {
        cb(AddChannelInfo{.channel = Address::makeFromId(333)});
      };

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
              const Bytes &,
              const TokenAmount &) -> outcome::result<TokenAmount> {
            return TokenAmount{0};
          }};
    }

    outcome::result<void> addPieceSample(
        const SamplePiece &piece, const std::shared_ptr<IpfsDatastore> &ipfs) {
        Bytes payload{"deadface"_unhex};
      OUTCOME_TRY(bytes, codec::cbor::encode(payload));
      OUTCOME_TRYA(payload_cid, common::getCidOf(bytes));
      CID piece_cid =
          "12209139839e65fabea9efd230898ad8b574509147e48d7c1e87a33d6da70fd2efae"_cid;
      deal = DealInfo{.deal_id = 18,
                      .sector_id = 4,
                      .offset = PaddedPieceSize(128),
                      .length = PaddedPieceSize(105)};
      PayloadLocation location{.relative_offset = 16, .block_size = 4};
      OUTCOME_TRY(piece_storage->addDealForPiece(piece_cid, deal));
      OUTCOME_TRY(piece_storage->addPayloadLocations(
          piece_cid, {{payload_cid, location}}));
      OUTCOME_TRY(setCbor(ipfs, payload));
      return outcome::success();
    }
  };
}  // namespace fc::markets::retrieval::test
