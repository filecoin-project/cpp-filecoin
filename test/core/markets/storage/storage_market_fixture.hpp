/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_MARKETS_STORAGE_STORAGE_MARKET_FIXTURE_HPP
#define CPP_FILECOIN_TEST_CORE_MARKETS_STORAGE_STORAGE_MARKET_FIXTURE_HPP

#include <gtest/gtest.h>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/security/plaintext.hpp>
#include "api/api.hpp"
#include "api/miner_api.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/impl/storage_market_client_impl.hpp"
#include "markets/storage/provider/impl/provider_impl.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/filestore/impl/filesystem/filesystem_filestore.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "testutil/literals.hpp"

namespace fc::markets::storage::test {
  using adt::Channel;
  using api::Api;
  using api::MarketBalance;
  using api::MinerApi;
  using api::MsgWait;
  using api::Wait;
  using client::StorageMarketClient;
  using client::StorageMarketClientImpl;
  using common::Buffer;
  using crypto::bls::BlsProvider;
  using crypto::bls::BlsProviderImpl;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::storage::InMemoryStorage;
  using fc::storage::filestore::FileStore;
  using fc::storage::filestore::FileSystemFileStore;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::InMemoryKeyStore;
  using fc::storage::keystore::KeyStore;
  using fc::storage::piece::PieceInfo;
  using libp2p::Host;
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::multi::Multiaddress;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::GasAmount;
  using primitives::cid::getCidOfCbor;
  using primitives::sector::RegisteredProof;
  using primitives::tipset::Tipset;
  using provider::Datastore;
  using provider::StorageProvider;
  using provider::StorageProviderImpl;
  using vm::VMExitCode;
  using vm::actor::builtin::market::PublishStorageDeals;
  using vm::actor::builtin::miner::MinerInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  /** Shared resources */
  static std::shared_ptr<libp2p::Host> host;
  static std::shared_ptr<boost::asio::io_context> context;
  static std::shared_ptr<Multiaddress> provider_multiaddress;

  class StorageMarketTest : public ::testing::Test {
   public:
    static const int kNumberOfWaitCycles = 50;  // 5 sec

    static void SetUpTestCase() {
      std::string address_string =
          "/ip4/127.0.0.1/tcp/40010/ipfs/"
          "12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV";

      // resulting PeerId should be
      // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
      KeyPair keypair{PublicKey{{Key::Type::Ed25519,
                                 "48453469c62f4885373099421a7365520b5ffb"
                                 "0d93726c124166be4b81d852e6"_unhex}},
                      PrivateKey{{Key::Type::Ed25519,
                                  "4a9361c525840f7086b893d584ebbe475b4ec"
                                  "7069951d2e897e8bceb0a3f35ce"_unhex}}};

      auto injector = libp2p::injector::makeHostInjector(
          libp2p::injector::useKeyPair(keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
      host = injector.create<std::shared_ptr<libp2p::Host>>();
      provider_multiaddress = std::make_shared<Multiaddress>(
          Multiaddress::create(address_string).value());
      OUTCOME_EXCEPT(host->listen(*provider_multiaddress));
      host->start();

      context = injector.create<std::shared_ptr<boost::asio::io_context>>();
    }

    static void TearDownTestCase() {
      host.reset();
      context.reset();
      provider_multiaddress.reset();
    }

    void SetUp() override {
      std::shared_ptr<BlsProvider> bls_provider =
          std::make_shared<BlsProviderImpl>();
      std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider =
          std::make_shared<Secp256k1Sha256ProviderImpl>();
      std::shared_ptr<Datastore> datastore =
          std::make_shared<InMemoryStorage>();
      std::shared_ptr<IpfsDatastore> ipfs_datastore =
          std::make_shared<InMemoryDatastore>();
      piece_io_ = std::make_shared<PieceIOImpl>(ipfs_datastore);

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
      auto miner_api = makeMinerApi();

      provider = makeProvider(*provider_multiaddress,
                              registered_proof,
                              miner_worker_keypair,
                              bls_provider,
                              secp256k1_provider,
                              datastore,
                              host,
                              context,
                              node_api,
                              miner_api,
                              miner_actor_address);
      OUTCOME_EXCEPT(provider->start());

      client = makeClient(client_keypair,
                          bls_provider,
                          secp256k1_provider,
                          host,
                          context,
                          datastore,
                          node_api);
      storage_provider_info = makeStorageProviderInfo(miner_actor_address,
                                                      miner_worker_address,
                                                      host,
                                                      *provider_multiaddress);

      logger->debug(
          "Provider info "
          + peerInfoToPrettyString(storage_provider_info.get()->peer_info));

      context->restart();
      std::thread([]() { context->run(); }).detach();
    }

    void TearDown() override {
      OUTCOME_EXCEPT(provider->stop());
      OUTCOME_EXCEPT(client->stop());
      context->stop();
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
    std::shared_ptr<Api> makeNodeApi(
        const Address &miner_actor_address,
        const BlsKeyPair &miner_worker_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::map<Address, Address> &account_keys,
        const std::map<Address, BlsKeyPair> &private_keys) {
      ChainEpoch epoch = 100;
      chain_head.height = epoch;

      std::shared_ptr<Api> api = std::make_shared<Api>();

      api->ChainGetMessage = {
          [this](const CID &message_cid) -> outcome::result<UnsignedMessage> {
            return this->messages[message_cid].message;
          }};

      api->StateLookupID = {[account_keys](const Address &address,
                                           auto &) -> outcome::result<Address> {
        if (address.isId()) {
          return address;
        }
        for (auto &[k, v] : account_keys) {
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
                             .pending_worker_key = boost::none,
                             .peer_id = fc::codec::cbor::kDefaultT<PeerId>(),
                             .seal_proof_type = {},
                             .sector_size = {},
                             .window_post_partition_sectors = {}};
          }};

      api->StateMarketBalance = {
          [this](auto &address,
                 auto &tipset_key) -> outcome::result<MarketBalance> {
            if (address == this->client_id_address) {
              return MarketBalance{2000000, 0};
            }
            throw "StateMarketBalance: wrong address";
          }};

      api->MarketEnsureAvailable = {
          [](auto, auto, auto, auto) -> outcome::result<boost::optional<CID>> {
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
              auto &unsigned_message) -> outcome::result<SignedMessage> {
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
              return signed_message;
            };
            throw "MpoolPushMessage: Wrong from address parameter";
          }};

      api->StateWaitMsg = {
          [this](auto &message_cid) -> outcome::result<Wait<MsgWait>> {
            logger->debug("StateWaitMsg called for message cid "
                          + message_cid.toString().value());
            PublishStorageDeals::Result publish_deal_result{};
            publish_deal_result.deals.emplace_back(1);
            auto publish_deal_result_encoded =
                codec::cbor::encode(publish_deal_result).value();

            MsgWait message_result{
                .receipt =
                    MessageReceipt{.exit_code = VMExitCode::Ok,
                                   .return_value = publish_deal_result_encoded,
                                   .gas_used = GasAmount{0}},
                .tipset = chain_head};
            auto channel = std::make_shared<Channel<Wait<MsgWait>::Result>>();
            channel->write(message_result);
            channel->closeWrite();
            Wait<MsgWait> wait_msg{channel};
            return wait_msg;
          }};

      api->WalletSign = {
          [private_keys, bls_provider](
              const Address &address,
              const Buffer &buffer) -> outcome::result<Signature> {
            auto it = private_keys.find(address);
            if (it == private_keys.end())
              throw "API WalletSign: address not found";
            return Signature{
                bls_provider->sign(buffer, it->second.private_key).value()};
          }};

      return api;
    }

    /**
     * Makes miner API
     * @return
     */
    std::shared_ptr<MinerApi> makeMinerApi() {
      std::shared_ptr<MinerApi> miner_api = std::make_shared<MinerApi>();

      miner_api->LocatePieceForDealWithinSector = {
          [](auto &deal_id, auto &tipset_key) -> outcome::result<PieceInfo> {
            return PieceInfo{};
          }};

      return miner_api;
    }

    std::shared_ptr<StorageProviderImpl> makeProvider(
        const Multiaddress &provider_multiaddress,
        const RegisteredProof &registered_proof,
        const BlsKeyPair &miner_worker_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
        const std::shared_ptr<Datastore> &datastore,
        const std::shared_ptr<libp2p::Host> &provider_host,
        const std::shared_ptr<boost::asio::io_context> &context,
        const std::shared_ptr<Api> &api,
        const std::shared_ptr<MinerApi> &miner_api,
        const Address &miner_actor_address) {
      std::shared_ptr<KeyStore> keystore =
          std::make_shared<InMemoryKeyStore>(bls_provider, secp256k1_provider);

      std::shared_ptr<FileStore> filestore =
          std::make_shared<FileSystemFileStore>();

      std::shared_ptr<StorageProviderImpl> new_provider =
          std::make_shared<StorageProviderImpl>(registered_proof,
                                                provider_host,
                                                context,
                                                keystore,
                                                datastore,
                                                api,
                                                miner_api,
                                                miner_actor_address,
                                                piece_io_,
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

    std::shared_ptr<StorageMarketClient> makeClient(
        const BlsKeyPair &client_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
        const std::shared_ptr<libp2p::Host> &client_host,
        const std::shared_ptr<boost::asio::io_context> &context,
        const std::shared_ptr<Datastore> &datastore,
        const std::shared_ptr<Api> &api) {
      std::shared_ptr<KeyStore> keystore =
          std::make_shared<InMemoryKeyStore>(bls_provider, secp256k1_provider);

      auto new_client = std::make_shared<StorageMarketClientImpl>(
          client_host, context, datastore, api, keystore, piece_io_);
      OUTCOME_EXCEPT(new_client->init());
      return new_client;
    }

    outcome::result<DataRef> makeDataRef(const CID &root_cid,
                                         const Buffer &data) {
      OUTCOME_TRY(piece_commitment,
                  piece_io_->generatePieceCommitment(registered_proof, data));
      return DataRef{.transfer_type = kTransferTypeManual,
                     .root = root_cid,
                     .piece_cid = piece_commitment.first,
                     .piece_size = piece_commitment.second};
    }

    /**
     * Waits for deal state in provider
     * @param proposal_cid - proposal deal cid
     * @param state - desired state
     */
    void waitForProviderDealStatus(const CID &proposal_cid,
                                   const StorageDealStatus &state) {
      for (int i = 0; i < kNumberOfWaitCycles; i++) {
        auto deal = provider->getDeal(proposal_cid);
        if (deal.has_value() && deal.value().state == state) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    /**
     * Waits for deal state in client
     * @param proposal_cid - proposal deal cid
     * @param state - desired state
     */
    void waitForClientDealStatus(const CID &proposal_cid,
                                 const StorageDealStatus &state) {
      for (int i = 0; i < kNumberOfWaitCycles; i++) {
        auto deal = client->getLocalDeal(proposal_cid);
        if (deal.has_value() && deal.value().state == state) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    common::Logger logger = common::createLogger("StorageMarketTest");

    Address miner_actor_address = Address::makeFromId(100);
    Address miner_worker_address;
    Address client_id_address = Address::makeFromId(102);
    Address client_bls_address;
    Tipset chain_head;
    std::shared_ptr<Api> node_api;
    std::shared_ptr<StorageMarketClient> client;
    std::shared_ptr<StorageProvider> provider;
    std::shared_ptr<StorageProviderInfo> storage_provider_info;

    RegisteredProof registered_proof{RegisteredProof::StackedDRG32GiBSeal};
    std::shared_ptr<PieceIO> piece_io_;

   private:
    // published messages
    std::map<CID, SignedMessage> messages;
  };

}  // namespace fc::markets::storage::test

#endif  // CPP_FILECOIN_TEST_CORE_MARKETS_STORAGE_STORAGE_MARKET_FIXTURE_HPP
