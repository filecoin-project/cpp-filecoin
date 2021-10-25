/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/security/plaintext.hpp>

#include "api/full_node/node_api.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/libp2p/soralog.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "data_transfer/dt.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/impl/storage_market_client_impl.hpp"
#include "markets/storage/provider/impl/provider_impl.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/car/car.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/filestore/impl/filesystem/filesystem_filestore.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/markets/storage/chain_events/chain_events_mock.hpp"
#include "testutil/mocks/miner/miner_mock.hpp"
#include "testutil/mocks/sectorblocks/blocks_mock.hpp"
#include "testutil/storage/base_fs_test.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"

namespace fc::markets::storage::test {
  using adt::Channel;
  using api::FullNodeApi;
  using api::MarketBalance;
  using api::MinerInfo;
  using api::MsgWait;
  using api::PieceLocation;
  using chain_events::ChainEvents;
  using chain_events::ChainEventsMock;
  using client::StorageMarketClient;
  using client::StorageMarketClientImpl;
  using client::import_manager::ImportManager;
  using crypto::bls::BlsProvider;
  using crypto::bls::BlsProviderImpl;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using data_transfer::DataTransfer;
  using fc::storage::InMemoryStorage;
  using fc::storage::filestore::FileStore;
  using fc::storage::filestore::FileSystemFileStore;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::ipfs::graphsync::GraphsyncImpl;
  using fc::storage::piece::PieceInfo;
  using libp2p::Host;
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::multi::Multiaddress;
  using miner::MinerMock;
  using miner::PieceAttributes;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::GasAmount;
  using primitives::kChainEpochUndefined;
  using primitives::cid::getCidOfCbor;
  using primitives::sector::RegisteredSealProof;
  using primitives::tipset::Tipset;
  using provider::Datastore;
  using provider::StorageProvider;
  using provider::StorageProviderImpl;
  using provider::StoredAsk;
  using sectorblocks::SectorBlocksMock;
  using vm::VMExitCode;
  using vm::actor::builtin::v0::market::PublishStorageDeals;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using BlsKeyPair = fc::crypto::bls::KeyPair;
  using testing::_;

  static auto port{40010};

  class StorageMarketTest : public ::test::BaseFS_Test {
   public:
    static constexpr auto kWaitTime = std::chrono::milliseconds(100);
    static const int kNumberOfWaitCycles = 50;  // 5 sec
    static inline const std::string kImportsTempDir = "storage_market_client";
    static inline const std::string kPieceIoTempDir = "piece_io";

    StorageMarketTest() : ::test::BaseFS_Test("storage_market_test") {}

    void SetUp() override {
      libp2pSoralog();

      createDir(kImportsTempDir);
      createDir(kPieceIoTempDir);

      std::string address_string = fmt::format(
          "/ip4/127.0.0.1/tcp/{}/ipfs/"
          "12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV",
          port++);

      // resulting PeerId should be
      // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
      KeyPair keypair{PublicKey{{Key::Type::Ed25519,
                                 "48453469c62f4885373099421a7365520b5ffb"
                                 "0d93726c124166be4b81d852e6"_unhex}},
                      PrivateKey{{Key::Type::Ed25519,
                                  "4a9361c525840f7086b893d584ebbe475b4ec"
                                  "7069951d2e897e8bceb0a3f35ce"_unhex}}};

      auto injector = libp2p::injector::makeHostInjector<
          boost::di::extension::shared_config>(
          libp2p::injector::useKeyPair(keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
      host = injector.create<std::shared_ptr<libp2p::Host>>();
      auto provider_multiaddress = std::make_shared<Multiaddress>(
          Multiaddress::create(address_string).value());
      OUTCOME_EXCEPT(host->listen(*provider_multiaddress));
      host->start();
      context = injector.create<std::shared_ptr<boost::asio::io_context>>();

      context_ = context;
      std::shared_ptr<BlsProvider> bls_provider =
          std::make_shared<BlsProviderImpl>();
      std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider =
          std::make_shared<Secp256k1Sha256ProviderImpl>();
      std::shared_ptr<Datastore> datastore =
          std::make_shared<InMemoryStorage>();
      ipld_provider = std::make_shared<InMemoryDatastore>();
      piece_io_ =
          std::make_shared<PieceIOImpl>(getPathString() / kPieceIoTempDir);

      import_manager =
          std::make_shared<ImportManager>(std::make_shared<InMemoryStorage>(),
                                          getPathString() / kImportsTempDir);

      OUTCOME_EXCEPT(miner_worker_keypair, bls_provider->generateKeyPair());
      miner_worker_address = Address::makeBls(miner_worker_keypair.public_key);
      OUTCOME_EXCEPT(client_keypair, bls_provider->generateKeyPair());

      std::map<Address, Address> account_keys;
      client_bls_address = Address::makeBls(client_keypair.public_key);
      account_keys[client_id_address] = client_bls_address;
      account_keys[miner_actor_address] = miner_worker_address;

      std::map<Address, BlsKeyPair> private_keys;
      private_keys[miner_worker_address] = miner_worker_keypair;
      private_keys[client_bls_address] = client_keypair;

      node_api = makeNodeApi(miner_actor_address,
                             miner_worker_keypair,
                             bls_provider,
                             account_keys,
                             private_keys);
      sector_blocks = std::make_shared<SectorBlocksMock>();

      EXPECT_CALL(*sector_blocks, addPiece(_, _, _))
          .WillRepeatedly(testing::Return(outcome::success(PieceAttributes{})));

      EXPECT_CALL(*sector_blocks, getRefs(_))
          .WillRepeatedly(testing::Return(
              outcome::success(std::vector({PieceLocation{}}))));

      miner = std::make_shared<MinerMock>();
      std::shared_ptr<mining::types::SectorInfo> state =
          std::make_shared<mining::types::SectorInfo>();
      state->state = mining::SealingState::kProving;
      EXPECT_CALL(*miner, getSectorInfo(_))
          .WillRepeatedly(testing::Return(outcome::success(state)));

      EXPECT_CALL(*sector_blocks, getMiner())
          .WillRepeatedly(testing::Return(miner));

      auto graphsync{
          std::make_shared<fc::storage::ipfs::graphsync::GraphsyncImpl>(
              host,
              injector.create<std::shared_ptr<libp2p::basic::Scheduler>>())};
      graphsync->subscribe([this](auto &from, auto &data) {
        OUTCOME_EXCEPT(ipld_provider->set(data.cid, BytesIn{data.content}));
      });
      graphsync->start();
      datatransfer = DataTransfer::make(host, graphsync);

      provider = makeProvider(*provider_multiaddress,
                              miner_worker_keypair,
                              bls_provider,
                              secp256k1_provider,
                              datastore,
                              host,
                              context_,
                              node_api,
                              sector_blocks,
                              chain_events_,
                              miner_actor_address);
      OUTCOME_EXCEPT(provider->start());

      client = makeClient(client_keypair,
                          bls_provider,
                          secp256k1_provider,
                          host,
                          context_,
                          datastore,
                          node_api);
      storage_provider_info = makeStorageProviderInfo(miner_actor_address,
                                                      miner_worker_address,
                                                      host,
                                                      *provider_multiaddress);

      logger->debug(
          "Provider info "
          + peerInfoToPrettyString(storage_provider_info.get()->peer_info));

      context_->restart();
    }

    void TearDown() override {
      OUTCOME_EXCEPT(provider->stop());
      OUTCOME_EXCEPT(client->stop());
    }

   protected:
    /**
     * Makes node API
     * @param miner_actor_address
     * @param miner_worker_keypair
     * @param provider_keypair
     * @param client_id_address
     * @param client_keypair
     * @param bls_provider
     * @return
     */
    std::shared_ptr<FullNodeApi> makeNodeApi(
        const Address &miner_actor_address,
        const BlsKeyPair &miner_worker_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::map<Address, Address> &account_keys,
        const std::map<Address, BlsKeyPair> &private_keys) {
      std::shared_ptr<FullNodeApi> api = std::make_shared<FullNodeApi>();

      api->ChainGetMessage = {
          [this](const CID &message_cid) -> outcome::result<UnsignedMessage> {
            return this->messages[message_cid].message;
          }};

      api->StateLookupID = {[account_keys](const Address &address,
                                           auto &) -> outcome::result<Address> {
        if (address.isId()) {
          return address;
        }
        for (const auto &[k, v] : account_keys) {
          if (v == address) {
            return k;
          }
        }
        throw "StateLookupID: address not found";
      }};

      api->ChainHead = {[this]() { return chain_head; }};

      api->StateMinerInfo = {
          [miner_actor_address](
              auto &address, auto &tipset_key) -> outcome::result<MinerInfo> {
            return MinerInfo{.owner = {},
                             .worker = miner_actor_address,
                             .control = {},
                             .peer_id = {},
                             .multiaddrs = {},
                             .window_post_proof_type = {},
                             .sector_size = {},
                             .window_post_partition_sectors = {}};
          }};

      api->GetProofType = {
          [this](auto &miner_address,
                 auto &tipset_key) -> outcome::result<RegisteredSealProof> {
            return registered_proof;
          }};

      api->StateMarketBalance = {
          [this](auto &address,
                 auto &tipset_key) -> outcome::result<MarketBalance> {
            if (address == this->client_id_address) {
              return MarketBalance{2000000, 0};
            }
            throw "StateMarketBalance: wrong address";
          }};

      api->MarketReserveFunds = {
          [](auto, auto, auto) -> outcome::result<boost::optional<CID>> {
            // funds ensured
            return boost::none;
          }};

      api->StateAccountKey = {
          [account_keys](auto &address,
                         auto &tipset_key) -> outcome::result<Address> {
            if (address.isKeyType()) return address;
            auto it = account_keys.find(address);
            if (it == account_keys.end())
              throw "StateAccountKey: address not found";
            return it->second;
          }};

      api->MpoolPushMessage = {
          [this, bls_provider, miner_worker_keypair, miner_actor_address](
              auto &unsigned_message, auto) -> outcome::result<SignedMessage> {
            if (unsigned_message.from == miner_actor_address) {
              OUTCOME_TRY(encoded_message,
                          codec::cbor::encode(unsigned_message));
              OUTCOME_TRY(signature,
                          bls_provider->sign(encoded_message,
                                             miner_worker_keypair.private_key));
              auto signed_message = SignedMessage{.message = unsigned_message,
                                                  .signature = signature};
              this->messages[signed_message.getCid()] = signed_message;
              this->logger->debug("MpoolPushMessage: message committed "
                                  + signed_message.getCid().toString().value());
              clientWaitsForProviderResponse();
              return signed_message;
            };
            throw "MpoolPushMessage: Wrong from address parameter";
          }};

      api->StateWaitMsg = {
          [this](
              auto &message_cid, auto, auto, auto) -> outcome::result<MsgWait> {
            logger->debug("StateWaitMsg called for message cid "
                          + message_cid.toString().value());
            PublishStorageDeals::Result publish_deal_result{};
            publish_deal_result.deals.emplace_back(1);
            auto publish_deal_result_encoded =
                codec::cbor::encode(publish_deal_result).value();

            MsgWait message_result{
                .message = message_cid,
                .receipt =
                    MessageReceipt{.exit_code = VMExitCode::kOk,
                                   .return_value = publish_deal_result_encoded,
                                   .gas_used = GasAmount{0}},
                .tipset = chain_head->key,
                .height = (ChainEpoch)chain_head->height(),
            };
            return message_result;
          }};

      std::weak_ptr<FullNodeApi> _api{api};
      api->WalletSign = {
          [=](const Address &address,
              const Bytes &buffer) -> outcome::result<Signature> {
            auto it = private_keys.find(
                _api.lock()->StateAccountKey(address, {}).value());
            if (it == private_keys.end())
              throw "API WalletSign: address not found";
            return Signature{
                bls_provider->sign(buffer, it->second.private_key).value()};
          }};

      api->WalletVerify = {
          [](const Address &address,
             const Bytes &buffer,
             const Signature &signature) -> outcome::result<bool> {
            return true;
          }};

      return api;
    }

    std::shared_ptr<StorageProviderImpl> makeProvider(
        const Multiaddress &provider_multiaddress,
        const BlsKeyPair &miner_worker_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
        const std::shared_ptr<Datastore> &datastore,
        const std::shared_ptr<libp2p::Host> &provider_host,
        const std::shared_ptr<boost::asio::io_context> &context,
        const std::shared_ptr<FullNodeApi> &api,
        const std::shared_ptr<SectorBlocksMock> &sector_blocks,
        const std::shared_ptr<ChainEventsMock> &chain_events,
        const Address &miner_actor_address) {
      std::shared_ptr<FileStore> filestore =
          std::make_shared<FileSystemFileStore>();

      stored_ask = std::make_shared<markets::storage::provider::StoredAsk>(
          std::make_shared<InMemoryStorage>(), api, miner_actor_address);

      std::shared_ptr<StorageProviderImpl> new_provider =
          std::make_shared<StorageProviderImpl>(
              provider_host,
              ipld_provider,
              datatransfer,
              stored_ask,
              context,
              std::make_shared<fc::storage::piece::PieceStorageImpl>(datastore),
              api,
              sector_blocks,
              chain_events,
              miner_actor_address,
              std::make_shared<PieceIOImpl>(kStorageMarketImportDir),
              filestore);
      OUTCOME_EXCEPT(new_provider->init());
      return new_provider;
    }

    std::shared_ptr<StorageProviderInfo> makeStorageProviderInfo(
        const Address &miner_actor_address,
        const Address &miner_worker_address,
        const std::shared_ptr<Host> &provider_host,
        const Multiaddress &multi_address) {
      return std::make_shared<StorageProviderInfo>(StorageProviderInfo{
          .address = miner_actor_address,
          .owner = {},
          .worker = miner_worker_address,
          .sector_size = SectorSize{1000000},
          .peer_info = PeerInfo{provider_host->getId(), {multi_address}}});
    }

    std::shared_ptr<StorageMarketClientImpl> makeClient(
        const BlsKeyPair &client_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
        const std::shared_ptr<libp2p::Host> &client_host,
        const std::shared_ptr<boost::asio::io_context> &context,
        const std::shared_ptr<Datastore> &datastore,
        const std::shared_ptr<FullNodeApi> &api) {
      auto new_client = std::make_shared<StorageMarketClientImpl>(
          client_host,
          context,
          import_manager,
          datatransfer,
          std::make_shared<markets::discovery::DiscoveryImpl>(datastore),
          api,
          chain_events_,
          piece_io_);
      OUTCOME_EXCEPT(new_client->init());
      return new_client;
    }

    outcome::result<DataRef> makeDataRef(
        const boost::filesystem::path &file_path) {
      OUTCOME_TRY(root, import_manager->import(file_path, true));
      OUTCOME_TRY(piece_commitment,
                  piece_io_->generatePieceCommitment(registered_proof,
                                                     file_path.string()));
      return DataRef{.transfer_type = kTransferTypeManual,
                     .root = root,
                     .piece_cid = piece_commitment.first,
                     .piece_size = piece_commitment.second};
    }

    /**
     * Waits for deal state in provider
     * @param proposal_cid - proposal deal cid
     * @param state - desired state
     * @returns true if expected state is met
     */
    bool waitForProviderDealStatus(const CID &proposal_cid,
                                   const StorageDealStatus &state) {
      for (int i = 0; i < kNumberOfWaitCycles; i++) {
        context_->run_for(kWaitTime);
        auto deal = provider->getDeal(proposal_cid);
        if (deal.has_value() && deal.value().state == state) {
          return true;
        }
      }
      return false;
    }

    /**
     * Wait for future result or timeout
     * @param future
     */
    void waitForAskResponse(
        std::future<outcome::result<SignedStorageAsk>> &future) {
      for (int i = 0; i < kNumberOfWaitCycles; i++) {
        context_->run_for(kWaitTime);
        if (future.wait_for(std::chrono::seconds(0))
            == std::future_status::ready)
          break;
      }
    }

    /**
     * Waits for deal state in client
     * @param proposal_cid - proposal deal cid
     * @param expected - desired state
     * @returns true if expected state is met
     */
    bool waitForClientDealStatus(const CID &proposal_cid,
                                 const StorageDealStatus &expected) {
      StorageDealStatus last_state;
      for (int i = 0; i < kNumberOfWaitCycles; i++) {
        context_->run_for(kWaitTime);
        auto deal = client->getLocalDeal(proposal_cid);
        if (deal.has_value()) {
          if (deal.value().state == expected) {
            return true;
          }
          last_state = deal.value().state;
        }
      }
      EXPECT_EQ(last_state, expected) << "Deal status doesn't match expected";
      return false;
    }

    /**
     * Awaits terminal statuses for all ongoing deals.
     */
    void clientWaitsForProviderResponse() {
      this->context_->post([this] { this->client->pollWaiting(); });
    }

    common::Logger logger = common::createLogger("StorageMarketTest");

    std::shared_ptr<libp2p::Host> host;
    std::shared_ptr<boost::asio::io_context> context;
    Address miner_actor_address = Address::makeFromId(100);
    Address miner_worker_address;
    Address client_id_address = Address::makeFromId(102);
    Address client_bls_address;
    std::shared_ptr<Tipset> chain_head = std::make_shared<Tipset>();
    std::shared_ptr<FullNodeApi> node_api;
    std::shared_ptr<ChainEventsMock> chain_events_ =
        std::make_shared<ChainEventsMock>();
    std::shared_ptr<SectorBlocksMock> sector_blocks;
    std::shared_ptr<MinerMock> miner;
    std::shared_ptr<StorageMarketClientImpl> client;
    std::shared_ptr<StorageProvider> provider;
    std::shared_ptr<StoredAsk> stored_ask;
    IpldPtr ipld_provider;
    std::shared_ptr<StorageProviderInfo> storage_provider_info;
    std::shared_ptr<DataTransfer> datatransfer;
    std::shared_ptr<ImportManager> import_manager;
    RegisteredSealProof registered_proof{
        RegisteredSealProof::kStackedDrg2KiBV1};
    std::shared_ptr<PieceIO> piece_io_;
    std::shared_ptr<boost::asio::io_context> context_;

   private:
    // published messages
    std::map<CID, SignedMessage> messages;
  };

}  // namespace fc::markets::storage::test
